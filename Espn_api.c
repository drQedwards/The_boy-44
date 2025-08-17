
/*
 * espn_api.c
 *
 * Fetch players wearing a given jersey number from ESPN's public endpoints.
 * Example:
 *   gcc espn_api.c -o espn_api -lcurl
 *   ./espn_api basketball nba 44 --active
 *
 * Endpoints used (public, no keys required):
 *   - Teams list:
 *     https://site.api.espn.com/apis/site/v2/sports/{sport}/{league}/teams?limit=500
 *   - Team with roster:
 *     https://site.api.espn.com/apis/site/v2/sports/{sport}/{league}/teams/{teamId}?enable=roster
 *
 * NOTE: This is a demo client. ESPN endpoints and shapes can change.
 *       Use responsibly and respect provider terms.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curl/curl.h>

/* -------------------------------
   Minimal dynamic buffer
   ------------------------------- */
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} s_buf;

static void sbuf_init(s_buf *b) { b->data=NULL; b->len=0; b->cap=0; }
static void sbuf_free(s_buf *b) { free(b->data); b->data=NULL; b->len=b->cap=0; }

static int sbuf_grow(s_buf *b, size_t need) {
    if (b->len + need + 1 <= b->cap) return 0;
    size_t ncap = b->cap? b->cap*2 : 4096;
    while (ncap < b->len + need + 1) ncap *= 2;
    char *nd = (char*)realloc(b->data, ncap);
    if (!nd) return -1;
    b->data = nd; b->cap = ncap;
    return 0;
}

static size_t curl_write_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    s_buf *b = (s_buf*)userdata;
    size_t n = size*nmemb;
    if (sbuf_grow(b, n) != 0) return 0;
    memcpy(b->data + b->len, ptr, n);
    b->len += n;
    b->data[b->len] = '\\0';
    return n;
}

static int http_get(const char *url, s_buf *out) {
    CURL *curl = curl_easy_init();
    if (!curl) return -1;
    sbuf_init(out);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "boy-44/1.1.1 (+https://github.com/drQedwards/boy-44)");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);
    CURLcode rc = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    if (rc != CURLE_OK || http_code < 200 || http_code >= 300) {
        fprintf(stderr, "HTTP GET failed (%ld): %s\\n", http_code, url);
        return -2;
    }
    return 0;
}

/* -------------------------------
   JSMN: Minimalistic JSON parser
   (https://github.com/zserge/jsmn) MIT License
   Single-file embed for convenience.
   ------------------------------- */
typedef enum {
    JSMN_UNDEFINED = 0,
    JSMN_OBJECT = 1,
    JSMN_ARRAY = 2,
    JSMN_STRING = 3,
    JSMN_PRIMITIVE = 4
} jsmntype_t;

typedef struct {
    jsmntype_t type;
    int start;
    int end;
    int size;
#ifdef JSMN_PARENT_LINKS
    int parent;
#endif
} jsmntok_t;

typedef struct {
    unsigned int pos;     /* offset in the JSON string */
    unsigned int toknext; /* next token to allocate */
    int toksuper;         /* superior token node, e.g. parent object or array */
} jsmn_parser;

static void jsmn_init(jsmn_parser *parser);
static int jsmn_parse(jsmn_parser *parser, const char *js, size_t len,
                      jsmntok_t *tokens, unsigned int num_tokens);

/* JSMN implementation (abridged) */
#ifndef JSMN_HEADER
#define JSMN_HEADER
static jsmntok_t jsmn_make_token(jsmntype_t type, int start, int end) {
    jsmntok_t t; t.type=type; t.start=start; t.end=end; t.size=0;
#ifdef JSMN_PARENT_LINKS
    t.parent = -1;
#endif
    return t;
}

static void jsmn_init(jsmn_parser *parser) {
    parser->pos = 0;
    parser->toknext = 0;
    parser->toksuper = -1;
}

static int jsmn_alloc_token(jsmn_parser *parser, jsmntok_t *tokens, size_t num_tokens) {
    if (parser->toknext >= num_tokens) return -1;
    tokens[parser->toknext] = jsmn_make_token(JSMN_UNDEFINED, -1, -1);
    return parser->toknext++;
}

static void jsmn_fill_token(jsmntok_t *token, jsmntype_t type, int start, int end) {
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;
}

static int jsmn_parse_primitive(jsmn_parser *parser, const char *js, size_t len,
                                jsmntok_t *tokens, size_t num_tokens) {
    int start = parser->pos;
    for (; parser->pos < (int)len; parser->pos++) {
        switch (js[parser->pos]) {
            case ':': case '\\t': case '\\r': case '\\n': case ' ':
            case ',': case ']': case '}':
                goto end;
        }
    }
end:
    if (tokens == NULL) { parser->pos--; return 0; }
    {
        int tok = jsmn_alloc_token(parser, tokens, num_tokens);
        if (tok < 0) { parser->pos--; return -1; }
        jsmn_fill_token(&tokens[tok], JSMN_PRIMITIVE, start, parser->pos);
    }
    parser->pos--;
    return 0;
}

static int jsmn_parse_string(jsmn_parser *parser, const char *js, size_t len,
                             jsmntok_t *tokens, size_t num_tokens) {
    int start = parser->pos;
    parser->pos++;
    for (; parser->pos < (int)len; parser->pos++) {
        char c = js[parser->pos];
        if (c == '\"') {
            if (tokens == NULL) return 0;
            {
                int tok = jsmn_alloc_token(parser, tokens, num_tokens);
                if (tok < 0) return -1;
                jsmn_fill_token(&tokens[tok], JSMN_STRING, start+1, parser->pos);
            }
            return 0;
        }
        if (c == '\\\\') parser->pos++;
    }
    return -1;
}

static int jsmn_parse(jsmn_parser *parser, const char *js, size_t len,
                      jsmntok_t *tokens, unsigned int num_tokens) {
    for (; parser->pos < (int)len; parser->pos++) {
        char c = js[parser->pos];
        switch (c) {
        case '{': case '[': {
            int tok = jsmn_alloc_token(parser, tokens, num_tokens);
            if (tok < 0) return -1;
            tokens[tok].type = (c == '{') ? JSMN_OBJECT : JSMN_ARRAY;
            tokens[tok].start = parser->pos;
            tokens[tok].end = -1;
            tokens[tok].size = 0;
            parser->toksuper = tok;
            break;
        }
        case '}': case ']': {
            int i;
            for (i = (int)parser->toknext - 1; i >= 0; i--) {
                if (tokens[i].start != -1 && tokens[i].end == -1) { tokens[i].end = parser->pos + 1; break; }
            }
            for (; i >= 0; i--) {
                if (tokens[i].start != -1 && tokens[i].end == -1) { parser->toksuper = i; break; }
            }
            break;
        }
        case '\"':
            if (jsmn_parse_string(parser, js, len, tokens, num_tokens) < 0) return -1;
            break;
        case '\\t': case '\\r': case '\\n': case ' ':
            break;
        case ':':
            parser->toksuper = parser->toknext - 1;
            break;
        case ',':
            break;
        default:
            if (jsmn_parse_primitive(parser, js, len, tokens, num_tokens) < 0) return -1;
            break;
        }
    }
    return parser->toknext;
}
#endif /* JSMN_HEADER */

/* -------------------------------
   JSON helpers
   ------------------------------- */
static int json_token_streq(const char *json, const jsmntok_t *t, const char *s) {
    int len = t->end - t->start;
    return (int)strlen(s) == len && strncmp(json + t->start, s, len) == 0;
}
static char* json_token_strdup(const char *json, const jsmntok_t *t) {
    int len = t->end - t->start;
    char *s = (char*)malloc(len+1);
    memcpy(s, json + t->start, len);
    s[len] = 0;
    return s;
}

/* Scan object token for key; return value token index or -1 */
static int json_find_value(const char *js, jsmntok_t *tok, int i, const char *key) {
    if (tok[i].type != JSMN_OBJECT) return -1;
    int pairs = tok[i].size;
    int k = i + 1;
    for (int p=0; p<pairs; p++) {
        int keytok = k, valtok = k+1;
        if (tok[keytok].type == JSMN_STRING && json_token_streq(js, &tok[keytok], key)) {
            return valtok;
        }
        /* naive skip to next pair */
        k = valtok + 1;
    }
    return -1;
}

/* -------------------------------
   ESPN parsers
   ------------------------------- */
typedef struct { char **ids; char **names; int count; } TeamList;
static void teamlist_free(TeamList *tl) {
    if (!tl) return;
    for (int i=0;i<tl->count;i++){ free(tl->ids[i]); free(tl->names[i]); }
    free(tl->ids); free(tl->names);
    tl->ids = tl->names = NULL; tl->count=0;
}

static TeamList parse_teams(const char *js, size_t len) {
    TeamList out = {0};
    jsmn_parser p; jsmn_init(&p);
    int cap = 16384;
    jsmntok_t *toks = (jsmntok_t*)calloc(cap, sizeof(*toks));
    if (!toks) return out;
    jsmn_parse(&p, js, len, toks, cap);

    out.ids = (char**)calloc(256, sizeof(char*));
    out.names = (char**)calloc(256, sizeof(char*));
    int out_cap = 256, out_cnt = 0;

    for (int i=1; i<cap-6; i++) {
        if (toks[i].type == JSMN_STRING && toks[i+1].type == JSMN_OBJECT) {
            if (json_token_streq(js, &toks[i], "team")) {
                int obj = i+1;
                int idv = json_find_value(js, toks, obj, "id");
                int dnv = json_find_value(js, toks, obj, "displayName");
                if (idv > 0 && dnv > 0 && toks[idv].type == JSMN_STRING && toks[dnv].type == JSMN_STRING) {
                    if (out_cnt == out_cap) {
                        out_cap *= 2;
                        out.ids = (char**)realloc(out.ids, out_cap*sizeof(char*));
                        out.names = (char**)realloc(out.names, out_cap*sizeof(char*));
                    }
                    out.ids[out_cnt] = json_token_strdup(js, &toks[idv]);
                    out.names[out_cnt] = json_token_strdup(js, &toks[dnv]);
                    out_cnt++;
                }
            }
        }
    }
    out.count = out_cnt;
    free(toks);
    return out;
}

static void print_roster_hits(const char *team_name, const char *js, size_t len, int number, int active_only) {
    jsmn_parser p; jsmn_init(&p);
    int cap = 32768;
    jsmntok_t *toks = (jsmntok_t*)calloc(cap, sizeof(*toks));
    if (!toks) return;
    jsmn_parse(&p, js, len, toks, cap);

    char needle[32]; snprintf(needle, sizeof(needle), "%d", number);
    for (int i=1; i<cap-6; i++) {
        if (toks[i].type != JSMN_OBJECT) continue;
        int fullv = json_find_value(js, toks, i, "fullName");
        int jerv  = json_find_value(js, toks, i, "jersey");
        if (fullv > 0 && jerv > 0 && toks[fullv].type == JSMN_STRING && toks[jerv].type == JSMN_STRING) {
            if (!json_token_streq(js, &toks[jerv], needle)) continue;
            /* optional active filter */
            int status_obj = json_find_value(js, toks, i, "status");
            int stat_name = (status_obj>0 && toks[status_obj].type==JSMN_OBJECT) ? json_find_value(js, toks, status_obj, "name") : -1;
            int is_active = 1;
            if (stat_name > 0 && toks[stat_name].type == JSMN_STRING) {
                char *sn = json_token_strdup(js, &toks[stat_name]);
                for (char *p=sn; *p; ++p) *p = toupper((unsigned char)*p);
                if (strstr(sn, "INACTIVE")) is_active = 0;
                free(sn);
            }
            if (active_only && !is_active) continue;

            char *name = json_token_strdup(js, &toks[fullv]);
            printf("%s â %s (#%s)\n", team_name, name, needle);
            free(name);
        }
    }
    free(toks);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <sport> <league> <number> [--active]\n", argv[0]);
        fprintf(stderr, "Example: %s basketball nba 44 --active\n", argv[0]);
        return 1;
    }
    const char *sport = argv[1];
    const char *league = argv[2];
    int number = atoi(argv[3]);
    int active_only = 0;
    for (int i=4;i<argc;i++) if (strcmp(argv[i],"--active")==0) active_only = 1;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    char url[512];
    snprintf(url, sizeof(url),
        "https://site.api.espn.com/apis/site/v2/sports/%s/%s/teams?limit=500",
        sport, league);

    s_buf teams; 
    if (http_get(url, &teams) != 0) { curl_global_cleanup(); return 2; }

    TeamList tl = parse_teams(teams.data, teams.len);
    if (tl.count == 0) {
        fprintf(stderr, "No teams parsed. Endpoint may have changed.\n");
        sbuf_free(&teams); curl_global_cleanup(); return 3;
    }
    fprintf(stderr, "Fetched %d teams; scanning rosters for #%d%s â¦\n",
            tl.count, number, active_only? " (active only)":"");

    for (int i=0; i<tl.count; i++) {
        char roster_url[512];
        snprintf(roster_url, sizeof(roster_url),
            "https://site.api.espn.com/apis/site/v2/sports/%s/%s/teams/%s?enable=roster",
            sport, league, tl.ids[i]);
        s_buf rjson;
        if (http_get(roster_url, &rjson) == 0) {
            print_roster_hits(tl.names[i], rjson.data, rjson.len, number, active_only);
            sbuf_free(&rjson);
        }
    }

    teamlist_free(&tl);
    sbuf_free(&teams);
    curl_global_cleanup();
    return 0;
}

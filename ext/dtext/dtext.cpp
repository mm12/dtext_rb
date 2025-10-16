
#line 1 "ext/dtext/dtext.cpp.rl"
#include "dtext.h"

#include <string.h>
#include <algorithm>
#include <cctype>
#include <tuple>

#ifdef DEBUG
#undef g_debug
#define STRINGIFY(x) XSTRINGIFY(x)
#define XSTRINGIFY(x) #x
#define g_debug(fmt, ...) fprintf(stderr, "\x1B[1;32mDEBUG\x1B[0m %-28.28s %-24.24s " fmt "\n", __FILE__ ":" STRINGIFY(__LINE__), __func__, ##__VA_ARGS__)
#else
#undef g_debug
#define g_debug(...)
#endif

static const size_t MAX_STACK_DEPTH = 512;

// Characters that mark the end of a link.
//
// http://www.fileformat.info/info/unicode/category/Pe/list.htm
// http://www.fileformat.info/info/unicode/block/cjk_symbols_and_punctuation/list.htm
static char32_t boundary_characters[] = {
  0x0021, // '!' U+0021 EXCLAMATION MARK
  0x0029, // ')' U+0029 RIGHT PARENTHESIS
  0x002C, // ',' U+002C COMMA
  0x002E, // '.' U+002E FULL STOP
  0x003A, // ':' U+003A COLON
  0x003B, // ';' U+003B SEMICOLON
  0x003C, // '<' U+003C LESS-THAN SIGN
  0x003E, // '>' U+003E GREATER-THAN SIGN
  0x003F, // '?' U+003F QUESTION MARK
  0x005D, // ']' U+005D RIGHT SQUARE BRACKET
  0x007D, // '}' U+007D RIGHT CURLY BRACKET
  0x276D, // '❭' U+276D MEDIUM RIGHT-POINTING ANGLE BRACKET ORNAMENT
  0x3000, // '　' U+3000 IDEOGRAPHIC SPACE (U+3000)
  0x3001, // '、' U+3001 IDEOGRAPHIC COMMA (U+3001)
  0x3002, // '。' U+3002 IDEOGRAPHIC FULL STOP (U+3002)
  0x3008, // '〈' U+3008 LEFT ANGLE BRACKET (U+3008)
  0x3009, // '〉' U+3009 RIGHT ANGLE BRACKET (U+3009)
  0x300A, // '《' U+300A LEFT DOUBLE ANGLE BRACKET (U+300A)
  0x300B, // '》' U+300B RIGHT DOUBLE ANGLE BRACKET (U+300B)
  0x300C, // '「' U+300C LEFT CORNER BRACKET (U+300C)
  0x300D, // '」' U+300D RIGHT CORNER BRACKET (U+300D)
  0x300E, // '『' U+300E LEFT WHITE CORNER BRACKET (U+300E)
  0x300F, // '』' U+300F RIGHT WHITE CORNER BRACKET (U+300F)
  0x3010, // '【' U+3010 LEFT BLACK LENTICULAR BRACKET (U+3010)
  0x3011, // '】' U+3011 RIGHT BLACK LENTICULAR BRACKET (U+3011)
  0x3014, // '〔' U+3014 LEFT TORTOISE SHELL BRACKET (U+3014)
  0x3015, // '〕' U+3015 RIGHT TORTOISE SHELL BRACKET (U+3015)
  0x3016, // '〖' U+3016 LEFT WHITE LENTICULAR BRACKET (U+3016)
  0x3017, // '〗' U+3017 RIGHT WHITE LENTICULAR BRACKET (U+3017)
  0x3018, // '〘' U+3018 LEFT WHITE TORTOISE SHELL BRACKET (U+3018)
  0x3019, // '〙' U+3019 RIGHT WHITE TORTOISE SHELL BRACKET (U+3019)
  0x301A, // '〚' U+301A LEFT WHITE SQUARE BRACKET (U+301A)
  0x301B, // '〛' U+301B RIGHT WHITE SQUARE BRACKET (U+301B)
  0x301C, // '〜' U+301C WAVE DASH (U+301C)
  0xFF09, // '）' U+FF09 FULLWIDTH RIGHT PARENTHESIS
  0xFF3D, // '］' U+FF3D FULLWIDTH RIGHT SQUARE BRACKET
  0xFF5D, // '｝' U+FF5D FULLWIDTH RIGHT CURLY BRACKET
  0xFF60, // '｠' U+FF60 FULLWIDTH RIGHT WHITE PARENTHESIS
  0xFF63, // '｣' U+FF63 HALFWIDTH RIGHT CORNER BRACKET
};


#line 752 "ext/dtext/dtext.cpp.rl"



#line 69 "ext/dtext/dtext.cpp"
static const int dtext_start = 713;
static const int dtext_first_final = 713;
static const int dtext_error = -1;

static const int dtext_en_basic_inline = 727;
static const int dtext_en_inline = 729;
static const int dtext_en_inline_code = 783;
static const int dtext_en_code = 785;
static const int dtext_en_table = 787;
static const int dtext_en_main = 713;


#line 755 "ext/dtext/dtext.cpp.rl"

void StateMachine::dstack_push(element_t element) {
  dstack.push_back(element);
}

element_t StateMachine::dstack_pop() {
  if (dstack.empty()) {
    g_debug("dstack pop empty stack");
    return DSTACK_EMPTY;
  } else {
    auto element = dstack.back();
    dstack.pop_back();
    return element;
  }
}

element_t StateMachine::dstack_peek() {
  return dstack.empty() ? DSTACK_EMPTY : dstack.back();
}

bool StateMachine::dstack_check(element_t expected_element) {
  return dstack_peek() == expected_element;
}

// Return true if the given tag is currently open.
bool StateMachine::dstack_is_open(element_t element) {
  return std::find(dstack.begin(), dstack.end(), element) != dstack.end();
}

int StateMachine::dstack_count(element_t element) {
  return std::count(dstack.begin(), dstack.end(), element);
}

void StateMachine::append(const std::string_view c) {
  output += c;
}

void StateMachine::append(const char c) {
  output += c;
}

void StateMachine::append_block(const std::string_view s) {
  if (!options.f_inline) {
    append(s);
  }
}

void StateMachine::append_block(const char s) {
  if (!options.f_inline) {
    append(s);
  }
}

void StateMachine::append_html_escaped(char s) {
  switch (s) {
    case '<': append("&lt;"); break;
    case '>': append("&gt;"); break;
    case '&': append("&amp;"); break;
    case '"': append("&quot;"); break;
    default:  append(s);
  }
}

void StateMachine::append_html_escaped(const std::string_view input) {
  for (const unsigned char c : input) {
    append_html_escaped(c);
  }
}

void StateMachine::append_uri_escaped(const std::string_view uri_part, const char whitelist) {
  static const char hex[] = "0123456789ABCDEF";

  for (const unsigned char c : uri_part) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == whitelist) {
      append(c);
    } else {
      append('%');
      append(hex[c >> 4]);
      append(hex[c & 0x0F]);
    }
  }
}

void StateMachine::append_url(const char* url) {
  if ((url[0] == '/' || url[0] == '#') && !options.base_url.empty()) {
    append(options.base_url);
  }

  append(url);
}

void StateMachine::append_id_link(const char * title, const char * id_name, const char * url) {
  append("<a class=\"dtext-link dtext-id-link dtext-");
  append(id_name);
  append("-id-link\" href=\"");
  append_url(url);
  append_uri_escaped({ a1, a2 });
  append("\">");
  append(title);
  append(" #");
  append_html_escaped({ a1, a2 });
  append("</a>");
}

void StateMachine::append_unnamed_url(const std::string_view url) {
  append("<a rel=\"nofollow\" class=\"dtext-link\" href=\"");
  append_html_escaped(url);
  append("\">");
  append_html_escaped(url);
  append("</a>");
}

void StateMachine::append_named_url(const std::string_view url, const std::string_view title) {
  auto parsed_title = parse_basic_inline(title);

  if (url[0] == '/' || url[0] == '#') {
    append("<a rel=\"nofollow\" class=\"dtext-link\" href=\"");
    if (!options.base_url.empty()) {
      append(options.base_url);
    }
  } else {
    append("<a rel=\"nofollow\" class=\"dtext-link dtext-external-link\" href=\"");
  }

  append_html_escaped(url);
  append("\">");
  append(parsed_title);
  append("</a>");
}

void StateMachine::append_wiki_link(const std::string_view tag, const std::string_view title) {
  std::string normalized_tag = std::string(tag);
  std::transform(normalized_tag.begin(), normalized_tag.end(), normalized_tag.begin(), [](unsigned char c) { return c == ' ' ? '_' : std::tolower(c); });

  // FIXME: Take the anchor as an argument here
  if (tag[0] == '#') {
    append("<a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"#");
    append_uri_escaped(normalized_tag.substr(1, normalized_tag.size() - 1));
    append("\">");
  } else {
    append("<a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"");
    append_url("/wiki_pages/show_or_new?title=");
    append_uri_escaped(normalized_tag, '#');
    append("\">");
  }
  append_html_escaped(title);
  append("</a>");
}

void StateMachine::append_post_search_link(const std::string_view tag, const std::string_view title) {
  std::string normalized_tag = std::string(tag);
  std::transform(normalized_tag.begin(), normalized_tag.end(), normalized_tag.begin(), [](unsigned char c) { return std::tolower(c); });

  append("<a rel=\"nofollow\" class=\"dtext-link dtext-post-search-link\" href=\"");
  append_url("/posts?tags=");
  append_uri_escaped(normalized_tag);
  append("\">");
  append_html_escaped(title);
  append("</a>");
}

void StateMachine::append_section(const std::string_view summary, bool initially_open) {
  dstack_close_leaf_blocks();
  dstack_open_block(BLOCK_SECTION, "<details");
  if (initially_open) {
     append_block(" open");
  }
  append_block(">");
  append_block("<summary>");
  if (!summary.empty()) {
    append_html_escaped(summary);
  }
  append_block("</summary><div>");
}

void StateMachine::append_closing_p() {
  if (output.size() > 4 && output.ends_with("<br>")) {
    output.resize(output.size() - 4);
  }

  if (output.size() > 3 && output.ends_with("<p>")) {
    output.resize(output.size() - 3);
    return;
  }

  append_block("</p>");
}

void StateMachine::dstack_open_inline(element_t type, const char * html) {
  g_debug("push inline element [%d]: %s", type, html);

  dstack_push(type);
  append(html);
}

void StateMachine::dstack_open_block(element_t type, const char * html) {
  g_debug("push block element [%d]: %s", type, html);

  dstack_push(type);
  append_block(html);
}

void StateMachine::dstack_close_inline(element_t type, const char * close_html) {
  if (dstack_check(type)) {
    g_debug("pop inline element [%d]: %s", type, close_html);

    dstack_pop();
    append(close_html);
  } else {
    g_debug("ignored out-of-order closing inline tag [%d]", type);

    append({ ts, te });
  }
}

bool StateMachine::dstack_close_block(element_t type, const char * close_html) {
  if (dstack_check(type)) {
    g_debug("pop block element [%d]: %s", type, close_html);

    dstack_pop();
    append_block(close_html);
    return true;
  } else {
    g_debug("ignored out-of-order closing block tag [%d]", type);

    append_block({ ts, te });
    return false;
  }
}

// Close the last open tag.
void StateMachine::dstack_rewind() {
  element_t element = dstack_pop();

  switch(element) {
    case BLOCK_P: append_closing_p(); break;
    case INLINE_SPOILER: append("</span>"); break;
    case BLOCK_SPOILER: append_block("</div>"); break;
    case BLOCK_QUOTE: append_block("</blockquote>"); break;
    case BLOCK_SECTION: append_block("</div></details>"); break;
    case BLOCK_CODE: append_block("</pre>"); break;
    case BLOCK_TD: append_block("</td>"); break;
    case BLOCK_TH: append_block("</th>"); break;

    case INLINE_B: append("</strong>"); break;
    case INLINE_I: append("</em>"); break;
    case INLINE_U: append("</u>"); break;
    case INLINE_S: append("</s>"); break;
    case INLINE_SUB: append("</sub>"); break;
    case INLINE_SUP: append("</sup>"); break;
    case INLINE_COLOR: append("</span>"); break;

    case BLOCK_TABLE: append_block("</table>"); break;
    case BLOCK_THEAD: append_block("</thead>"); break;
    case BLOCK_TBODY: append_block("</tbody>"); break;
    case BLOCK_TR: append_block("</tr>"); header_mode = false; break;
    case BLOCK_UL: append_block("</ul>"); header_mode = false; break;
    case BLOCK_LI: append_block("</li>"); header_mode = false; break;
    case BLOCK_H6: append_block("</h6>"); header_mode = false; break;
    case BLOCK_H5: append_block("</h5>"); header_mode = false; break;
    case BLOCK_H4: append_block("</h4>"); header_mode = false; break;
    case BLOCK_H3: append_block("</h3>"); header_mode = false; break;
    case BLOCK_H2: append_block("</h2>"); header_mode = false; break;
    case BLOCK_H1: append_block("</h1>"); header_mode = false; break;

    case DSTACK_EMPTY: break;
  }
}

// Close the last open paragraph or list, if there is one.
void StateMachine::dstack_close_before_block() {
  while (dstack_check(BLOCK_P) || dstack_check(BLOCK_LI) || dstack_check(BLOCK_UL)) {
    dstack_rewind();
  }
}

// Close all remaining open tags.
void StateMachine::dstack_close_all() {
  while (!dstack.empty()) {
    dstack_rewind();
  }
}

// container blocks: [quote], [spoiler], [section]
// leaf blocks: [code], [table], [td]?, [th]?, <h1>, <p>, <li>, <ul>
void StateMachine::dstack_close_leaf_blocks() {
  g_debug("dstack close leaf blocks");

  while (!dstack.empty() && !dstack_check(BLOCK_QUOTE) && !dstack_check(BLOCK_SPOILER) && !dstack_check(BLOCK_SECTION)) {
    dstack_rewind();
  }
}

// Close all open tags up to and including the given tag.
void StateMachine::dstack_close_until(element_t element) {
  while (!dstack.empty() && !dstack_check(element)) {
    dstack_rewind();
  }

  dstack_rewind();
}

void StateMachine::dstack_open_list(int depth) {
  g_debug("open list");

  if (dstack_is_open(BLOCK_LI)) {
    dstack_close_until(BLOCK_LI);
  } else {
    dstack_close_leaf_blocks();
  }

  while (dstack_count(BLOCK_UL) < depth) {
    dstack_open_block(BLOCK_UL, "<ul>");
  }

  while (dstack_count(BLOCK_UL) > depth) {
    dstack_close_until( BLOCK_UL);
  }

  dstack_open_block(BLOCK_LI, "<li>");
}

void StateMachine::dstack_close_list() {
  while (dstack_is_open(BLOCK_UL)) {
    dstack_close_until(BLOCK_UL);
  }
}

static inline std::tuple<char32_t, int> get_utf8_char(const char* c) {
  const unsigned char* p = reinterpret_cast<const unsigned char*>(c);

  // 0x10xxxxxx is a continuation byte; back up to the leading byte.
  while ((p[0] >> 6) == 0b10) {
    p--;
  }

  if (p[0] >> 7 == 0) {
    // 0x0xxxxxxx
    return { p[0], 1 };
  } else if ((p[0] >> 5) == 0b110) {
    // 0x110xxxxx, 0x10xxxxxx
    return { ((p[0] & 0b00011111) << 6) | (p[1] & 0b00111111), 2 };
  } else if ((p[0] >> 4) == 0b1110) {
    // 0x1110xxxx, 0x10xxxxxx, 0x10xxxxxx
    return { ((p[0] & 0b00001111) << 12) | (p[1] & 0b00111111) << 6 | (p[2] & 0b00111111), 3 };
  } else if ((p[0] >> 3) == 0b11110) {
    // 0x11110xxx, 0x10xxxxxx, 0x10xxxxxx, 0x10xxxxxx
    return { ((p[0] & 0b00000111) << 18) | (p[1] & 0b00111111) << 12 | (p[2] & 0b00111111) << 6 | (p[3] & 0b00111111), 4 };
  } else {
    return { 0, 0 };
  }
}

// Returns the preceding non-boundary character if `c` is a boundary character.
// Otherwise, returns `c` if `c` is not a boundary character. Boundary characters
// are trailing punctuation characters that should not be part of the matched text.
static inline const char* find_boundary_c(const char* c) {
  auto [ch, len] = get_utf8_char(c);

  if (std::binary_search(std::begin(boundary_characters), std::end(boundary_characters), ch)) {
    return c - len;
  } else {
    return c;
  }
}

StateMachine::StateMachine(const std::string_view dtext, int initial_state, const DTextOptions options) : options(options) {
  output.reserve(dtext.size() * 1.5);
  stack.reserve(16);
  dstack.reserve(16);
  posts.reserve(10);

  p = dtext.data();
  pb = p;
  pe = p + dtext.size();
  eof = pe;
  cs = initial_state;
}

std::string StateMachine::parse_basic_inline(const std::string_view dtext) {
    DTextOptions options = {};
    options.f_inline = true;
    options.allow_color = false;
    options.max_thumbs = 0;

    StateMachine sm(dtext, dtext_en_basic_inline, options);

    return sm.parse().dtext;
}

DTextResult StateMachine::parse_dtext(const std::string_view dtext, DTextOptions options) {
  StateMachine sm(dtext, dtext_en_main, options);
  return sm.parse();
}

DTextResult StateMachine::parse() {
  StateMachine* sm = this;
  g_debug("start\n");

  
#line 479 "ext/dtext/dtext.cpp"
	{
	( sm->top) = 0;
	( sm->ts) = 0;
	( sm->te) = 0;
	( sm->act) = 0;
	}

#line 1155 "ext/dtext/dtext.cpp.rl"
  
#line 485 "ext/dtext/dtext.cpp"
	{
	short _widec;
	if ( ( sm->p) == ( sm->pe) )
		goto _test_eof;
	goto _resume;

_again:
	switch (  sm->cs ) {
		case 713: goto st713;
		case 714: goto st714;
		case 0: goto st0;
		case 715: goto st715;
		case 716: goto st716;
		case 1: goto st1;
		case 717: goto st717;
		case 718: goto st718;
		case 2: goto st2;
		case 719: goto st719;
		case 3: goto st3;
		case 720: goto st720;
		case 721: goto st721;
		case 4: goto st4;
		case 5: goto st5;
		case 6: goto st6;
		case 7: goto st7;
		case 8: goto st8;
		case 9: goto st9;
		case 10: goto st10;
		case 11: goto st11;
		case 12: goto st12;
		case 13: goto st13;
		case 14: goto st14;
		case 15: goto st15;
		case 16: goto st16;
		case 722: goto st722;
		case 17: goto st17;
		case 18: goto st18;
		case 19: goto st19;
		case 20: goto st20;
		case 21: goto st21;
		case 22: goto st22;
		case 23: goto st23;
		case 24: goto st24;
		case 25: goto st25;
		case 26: goto st26;
		case 27: goto st27;
		case 28: goto st28;
		case 29: goto st29;
		case 30: goto st30;
		case 31: goto st31;
		case 32: goto st32;
		case 33: goto st33;
		case 34: goto st34;
		case 35: goto st35;
		case 36: goto st36;
		case 37: goto st37;
		case 38: goto st38;
		case 39: goto st39;
		case 40: goto st40;
		case 41: goto st41;
		case 42: goto st42;
		case 43: goto st43;
		case 44: goto st44;
		case 45: goto st45;
		case 46: goto st46;
		case 47: goto st47;
		case 48: goto st48;
		case 49: goto st49;
		case 50: goto st50;
		case 51: goto st51;
		case 52: goto st52;
		case 53: goto st53;
		case 54: goto st54;
		case 55: goto st55;
		case 56: goto st56;
		case 57: goto st57;
		case 58: goto st58;
		case 59: goto st59;
		case 60: goto st60;
		case 61: goto st61;
		case 62: goto st62;
		case 63: goto st63;
		case 64: goto st64;
		case 65: goto st65;
		case 66: goto st66;
		case 67: goto st67;
		case 68: goto st68;
		case 69: goto st69;
		case 70: goto st70;
		case 71: goto st71;
		case 72: goto st72;
		case 73: goto st73;
		case 74: goto st74;
		case 75: goto st75;
		case 76: goto st76;
		case 77: goto st77;
		case 78: goto st78;
		case 79: goto st79;
		case 80: goto st80;
		case 81: goto st81;
		case 82: goto st82;
		case 83: goto st83;
		case 84: goto st84;
		case 85: goto st85;
		case 86: goto st86;
		case 87: goto st87;
		case 88: goto st88;
		case 89: goto st89;
		case 90: goto st90;
		case 91: goto st91;
		case 92: goto st92;
		case 93: goto st93;
		case 94: goto st94;
		case 95: goto st95;
		case 96: goto st96;
		case 97: goto st97;
		case 98: goto st98;
		case 99: goto st99;
		case 100: goto st100;
		case 101: goto st101;
		case 102: goto st102;
		case 103: goto st103;
		case 104: goto st104;
		case 105: goto st105;
		case 106: goto st106;
		case 107: goto st107;
		case 108: goto st108;
		case 109: goto st109;
		case 110: goto st110;
		case 111: goto st111;
		case 112: goto st112;
		case 113: goto st113;
		case 114: goto st114;
		case 115: goto st115;
		case 116: goto st116;
		case 117: goto st117;
		case 118: goto st118;
		case 119: goto st119;
		case 120: goto st120;
		case 121: goto st121;
		case 122: goto st122;
		case 123: goto st123;
		case 124: goto st124;
		case 125: goto st125;
		case 126: goto st126;
		case 127: goto st127;
		case 128: goto st128;
		case 129: goto st129;
		case 130: goto st130;
		case 131: goto st131;
		case 132: goto st132;
		case 723: goto st723;
		case 133: goto st133;
		case 134: goto st134;
		case 135: goto st135;
		case 136: goto st136;
		case 137: goto st137;
		case 138: goto st138;
		case 139: goto st139;
		case 140: goto st140;
		case 724: goto st724;
		case 141: goto st141;
		case 142: goto st142;
		case 143: goto st143;
		case 144: goto st144;
		case 145: goto st145;
		case 146: goto st146;
		case 147: goto st147;
		case 725: goto st725;
		case 148: goto st148;
		case 149: goto st149;
		case 150: goto st150;
		case 151: goto st151;
		case 152: goto st152;
		case 726: goto st726;
		case 727: goto st727;
		case 728: goto st728;
		case 153: goto st153;
		case 154: goto st154;
		case 155: goto st155;
		case 156: goto st156;
		case 157: goto st157;
		case 158: goto st158;
		case 159: goto st159;
		case 160: goto st160;
		case 161: goto st161;
		case 162: goto st162;
		case 163: goto st163;
		case 164: goto st164;
		case 165: goto st165;
		case 166: goto st166;
		case 167: goto st167;
		case 729: goto st729;
		case 730: goto st730;
		case 731: goto st731;
		case 168: goto st168;
		case 169: goto st169;
		case 170: goto st170;
		case 171: goto st171;
		case 172: goto st172;
		case 173: goto st173;
		case 174: goto st174;
		case 175: goto st175;
		case 176: goto st176;
		case 177: goto st177;
		case 178: goto st178;
		case 179: goto st179;
		case 180: goto st180;
		case 181: goto st181;
		case 182: goto st182;
		case 732: goto st732;
		case 733: goto st733;
		case 183: goto st183;
		case 184: goto st184;
		case 734: goto st734;
		case 185: goto st185;
		case 186: goto st186;
		case 187: goto st187;
		case 188: goto st188;
		case 189: goto st189;
		case 190: goto st190;
		case 191: goto st191;
		case 735: goto st735;
		case 192: goto st192;
		case 193: goto st193;
		case 194: goto st194;
		case 195: goto st195;
		case 196: goto st196;
		case 197: goto st197;
		case 198: goto st198;
		case 736: goto st736;
		case 737: goto st737;
		case 738: goto st738;
		case 199: goto st199;
		case 200: goto st200;
		case 201: goto st201;
		case 202: goto st202;
		case 739: goto st739;
		case 203: goto st203;
		case 204: goto st204;
		case 205: goto st205;
		case 206: goto st206;
		case 207: goto st207;
		case 208: goto st208;
		case 209: goto st209;
		case 210: goto st210;
		case 211: goto st211;
		case 212: goto st212;
		case 213: goto st213;
		case 214: goto st214;
		case 215: goto st215;
		case 216: goto st216;
		case 217: goto st217;
		case 218: goto st218;
		case 219: goto st219;
		case 740: goto st740;
		case 220: goto st220;
		case 221: goto st221;
		case 222: goto st222;
		case 223: goto st223;
		case 224: goto st224;
		case 225: goto st225;
		case 226: goto st226;
		case 227: goto st227;
		case 228: goto st228;
		case 741: goto st741;
		case 229: goto st229;
		case 230: goto st230;
		case 231: goto st231;
		case 232: goto st232;
		case 233: goto st233;
		case 234: goto st234;
		case 742: goto st742;
		case 235: goto st235;
		case 236: goto st236;
		case 237: goto st237;
		case 238: goto st238;
		case 239: goto st239;
		case 240: goto st240;
		case 241: goto st241;
		case 743: goto st743;
		case 744: goto st744;
		case 242: goto st242;
		case 243: goto st243;
		case 244: goto st244;
		case 245: goto st245;
		case 745: goto st745;
		case 246: goto st246;
		case 247: goto st247;
		case 248: goto st248;
		case 249: goto st249;
		case 250: goto st250;
		case 746: goto st746;
		case 251: goto st251;
		case 252: goto st252;
		case 253: goto st253;
		case 254: goto st254;
		case 747: goto st747;
		case 748: goto st748;
		case 255: goto st255;
		case 256: goto st256;
		case 257: goto st257;
		case 258: goto st258;
		case 259: goto st259;
		case 260: goto st260;
		case 261: goto st261;
		case 262: goto st262;
		case 749: goto st749;
		case 750: goto st750;
		case 263: goto st263;
		case 264: goto st264;
		case 265: goto st265;
		case 266: goto st266;
		case 267: goto st267;
		case 751: goto st751;
		case 268: goto st268;
		case 269: goto st269;
		case 270: goto st270;
		case 271: goto st271;
		case 272: goto st272;
		case 273: goto st273;
		case 752: goto st752;
		case 753: goto st753;
		case 274: goto st274;
		case 275: goto st275;
		case 276: goto st276;
		case 277: goto st277;
		case 278: goto st278;
		case 279: goto st279;
		case 754: goto st754;
		case 280: goto st280;
		case 755: goto st755;
		case 281: goto st281;
		case 282: goto st282;
		case 283: goto st283;
		case 284: goto st284;
		case 285: goto st285;
		case 286: goto st286;
		case 287: goto st287;
		case 288: goto st288;
		case 289: goto st289;
		case 290: goto st290;
		case 291: goto st291;
		case 292: goto st292;
		case 756: goto st756;
		case 757: goto st757;
		case 293: goto st293;
		case 294: goto st294;
		case 295: goto st295;
		case 296: goto st296;
		case 297: goto st297;
		case 298: goto st298;
		case 299: goto st299;
		case 300: goto st300;
		case 301: goto st301;
		case 302: goto st302;
		case 303: goto st303;
		case 758: goto st758;
		case 759: goto st759;
		case 304: goto st304;
		case 305: goto st305;
		case 306: goto st306;
		case 307: goto st307;
		case 308: goto st308;
		case 760: goto st760;
		case 761: goto st761;
		case 309: goto st309;
		case 310: goto st310;
		case 311: goto st311;
		case 312: goto st312;
		case 313: goto st313;
		case 762: goto st762;
		case 314: goto st314;
		case 315: goto st315;
		case 316: goto st316;
		case 317: goto st317;
		case 763: goto st763;
		case 318: goto st318;
		case 319: goto st319;
		case 320: goto st320;
		case 321: goto st321;
		case 322: goto st322;
		case 323: goto st323;
		case 324: goto st324;
		case 325: goto st325;
		case 326: goto st326;
		case 764: goto st764;
		case 765: goto st765;
		case 327: goto st327;
		case 328: goto st328;
		case 329: goto st329;
		case 330: goto st330;
		case 331: goto st331;
		case 332: goto st332;
		case 333: goto st333;
		case 766: goto st766;
		case 767: goto st767;
		case 334: goto st334;
		case 335: goto st335;
		case 336: goto st336;
		case 337: goto st337;
		case 768: goto st768;
		case 769: goto st769;
		case 338: goto st338;
		case 339: goto st339;
		case 340: goto st340;
		case 341: goto st341;
		case 342: goto st342;
		case 343: goto st343;
		case 344: goto st344;
		case 345: goto st345;
		case 346: goto st346;
		case 347: goto st347;
		case 770: goto st770;
		case 348: goto st348;
		case 349: goto st349;
		case 350: goto st350;
		case 351: goto st351;
		case 352: goto st352;
		case 353: goto st353;
		case 354: goto st354;
		case 355: goto st355;
		case 356: goto st356;
		case 357: goto st357;
		case 358: goto st358;
		case 359: goto st359;
		case 360: goto st360;
		case 361: goto st361;
		case 771: goto st771;
		case 362: goto st362;
		case 363: goto st363;
		case 364: goto st364;
		case 365: goto st365;
		case 366: goto st366;
		case 367: goto st367;
		case 368: goto st368;
		case 772: goto st772;
		case 369: goto st369;
		case 370: goto st370;
		case 371: goto st371;
		case 372: goto st372;
		case 373: goto st373;
		case 374: goto st374;
		case 773: goto st773;
		case 774: goto st774;
		case 375: goto st375;
		case 376: goto st376;
		case 377: goto st377;
		case 378: goto st378;
		case 379: goto st379;
		case 775: goto st775;
		case 776: goto st776;
		case 380: goto st380;
		case 381: goto st381;
		case 382: goto st382;
		case 383: goto st383;
		case 384: goto st384;
		case 777: goto st777;
		case 778: goto st778;
		case 385: goto st385;
		case 386: goto st386;
		case 387: goto st387;
		case 388: goto st388;
		case 389: goto st389;
		case 390: goto st390;
		case 391: goto st391;
		case 392: goto st392;
		case 779: goto st779;
		case 393: goto st393;
		case 394: goto st394;
		case 395: goto st395;
		case 396: goto st396;
		case 397: goto st397;
		case 398: goto st398;
		case 399: goto st399;
		case 400: goto st400;
		case 401: goto st401;
		case 402: goto st402;
		case 403: goto st403;
		case 404: goto st404;
		case 405: goto st405;
		case 780: goto st780;
		case 406: goto st406;
		case 407: goto st407;
		case 408: goto st408;
		case 409: goto st409;
		case 410: goto st410;
		case 411: goto st411;
		case 412: goto st412;
		case 413: goto st413;
		case 414: goto st414;
		case 415: goto st415;
		case 416: goto st416;
		case 417: goto st417;
		case 418: goto st418;
		case 419: goto st419;
		case 420: goto st420;
		case 421: goto st421;
		case 422: goto st422;
		case 423: goto st423;
		case 424: goto st424;
		case 425: goto st425;
		case 426: goto st426;
		case 427: goto st427;
		case 428: goto st428;
		case 429: goto st429;
		case 430: goto st430;
		case 431: goto st431;
		case 432: goto st432;
		case 433: goto st433;
		case 434: goto st434;
		case 435: goto st435;
		case 436: goto st436;
		case 437: goto st437;
		case 438: goto st438;
		case 439: goto st439;
		case 440: goto st440;
		case 441: goto st441;
		case 442: goto st442;
		case 443: goto st443;
		case 444: goto st444;
		case 445: goto st445;
		case 446: goto st446;
		case 447: goto st447;
		case 448: goto st448;
		case 449: goto st449;
		case 450: goto st450;
		case 451: goto st451;
		case 452: goto st452;
		case 453: goto st453;
		case 454: goto st454;
		case 455: goto st455;
		case 456: goto st456;
		case 457: goto st457;
		case 458: goto st458;
		case 459: goto st459;
		case 460: goto st460;
		case 461: goto st461;
		case 462: goto st462;
		case 463: goto st463;
		case 464: goto st464;
		case 465: goto st465;
		case 466: goto st466;
		case 467: goto st467;
		case 468: goto st468;
		case 469: goto st469;
		case 470: goto st470;
		case 471: goto st471;
		case 472: goto st472;
		case 473: goto st473;
		case 474: goto st474;
		case 475: goto st475;
		case 476: goto st476;
		case 477: goto st477;
		case 478: goto st478;
		case 479: goto st479;
		case 480: goto st480;
		case 481: goto st481;
		case 482: goto st482;
		case 483: goto st483;
		case 484: goto st484;
		case 485: goto st485;
		case 486: goto st486;
		case 487: goto st487;
		case 488: goto st488;
		case 489: goto st489;
		case 490: goto st490;
		case 491: goto st491;
		case 492: goto st492;
		case 493: goto st493;
		case 494: goto st494;
		case 495: goto st495;
		case 496: goto st496;
		case 497: goto st497;
		case 498: goto st498;
		case 499: goto st499;
		case 500: goto st500;
		case 501: goto st501;
		case 502: goto st502;
		case 503: goto st503;
		case 504: goto st504;
		case 505: goto st505;
		case 506: goto st506;
		case 507: goto st507;
		case 508: goto st508;
		case 509: goto st509;
		case 510: goto st510;
		case 511: goto st511;
		case 512: goto st512;
		case 513: goto st513;
		case 514: goto st514;
		case 515: goto st515;
		case 516: goto st516;
		case 517: goto st517;
		case 518: goto st518;
		case 519: goto st519;
		case 520: goto st520;
		case 521: goto st521;
		case 522: goto st522;
		case 523: goto st523;
		case 524: goto st524;
		case 525: goto st525;
		case 526: goto st526;
		case 527: goto st527;
		case 528: goto st528;
		case 529: goto st529;
		case 530: goto st530;
		case 531: goto st531;
		case 532: goto st532;
		case 533: goto st533;
		case 534: goto st534;
		case 535: goto st535;
		case 536: goto st536;
		case 537: goto st537;
		case 538: goto st538;
		case 539: goto st539;
		case 540: goto st540;
		case 541: goto st541;
		case 542: goto st542;
		case 543: goto st543;
		case 544: goto st544;
		case 545: goto st545;
		case 546: goto st546;
		case 547: goto st547;
		case 548: goto st548;
		case 549: goto st549;
		case 550: goto st550;
		case 551: goto st551;
		case 552: goto st552;
		case 553: goto st553;
		case 554: goto st554;
		case 555: goto st555;
		case 556: goto st556;
		case 557: goto st557;
		case 558: goto st558;
		case 559: goto st559;
		case 560: goto st560;
		case 561: goto st561;
		case 562: goto st562;
		case 563: goto st563;
		case 564: goto st564;
		case 565: goto st565;
		case 566: goto st566;
		case 567: goto st567;
		case 568: goto st568;
		case 569: goto st569;
		case 570: goto st570;
		case 571: goto st571;
		case 572: goto st572;
		case 573: goto st573;
		case 574: goto st574;
		case 575: goto st575;
		case 576: goto st576;
		case 577: goto st577;
		case 578: goto st578;
		case 579: goto st579;
		case 580: goto st580;
		case 581: goto st581;
		case 582: goto st582;
		case 583: goto st583;
		case 584: goto st584;
		case 585: goto st585;
		case 586: goto st586;
		case 587: goto st587;
		case 588: goto st588;
		case 589: goto st589;
		case 590: goto st590;
		case 591: goto st591;
		case 592: goto st592;
		case 593: goto st593;
		case 594: goto st594;
		case 595: goto st595;
		case 596: goto st596;
		case 597: goto st597;
		case 598: goto st598;
		case 599: goto st599;
		case 600: goto st600;
		case 601: goto st601;
		case 602: goto st602;
		case 603: goto st603;
		case 604: goto st604;
		case 605: goto st605;
		case 606: goto st606;
		case 607: goto st607;
		case 608: goto st608;
		case 609: goto st609;
		case 610: goto st610;
		case 611: goto st611;
		case 612: goto st612;
		case 613: goto st613;
		case 614: goto st614;
		case 615: goto st615;
		case 616: goto st616;
		case 617: goto st617;
		case 618: goto st618;
		case 619: goto st619;
		case 620: goto st620;
		case 621: goto st621;
		case 622: goto st622;
		case 623: goto st623;
		case 624: goto st624;
		case 625: goto st625;
		case 626: goto st626;
		case 627: goto st627;
		case 628: goto st628;
		case 629: goto st629;
		case 630: goto st630;
		case 631: goto st631;
		case 632: goto st632;
		case 633: goto st633;
		case 634: goto st634;
		case 635: goto st635;
		case 636: goto st636;
		case 637: goto st637;
		case 638: goto st638;
		case 639: goto st639;
		case 640: goto st640;
		case 641: goto st641;
		case 642: goto st642;
		case 643: goto st643;
		case 644: goto st644;
		case 645: goto st645;
		case 646: goto st646;
		case 647: goto st647;
		case 648: goto st648;
		case 649: goto st649;
		case 650: goto st650;
		case 651: goto st651;
		case 652: goto st652;
		case 653: goto st653;
		case 654: goto st654;
		case 655: goto st655;
		case 656: goto st656;
		case 657: goto st657;
		case 658: goto st658;
		case 659: goto st659;
		case 660: goto st660;
		case 661: goto st661;
		case 662: goto st662;
		case 663: goto st663;
		case 664: goto st664;
		case 665: goto st665;
		case 666: goto st666;
		case 667: goto st667;
		case 668: goto st668;
		case 669: goto st669;
		case 670: goto st670;
		case 671: goto st671;
		case 672: goto st672;
		case 673: goto st673;
		case 674: goto st674;
		case 781: goto st781;
		case 782: goto st782;
		case 675: goto st675;
		case 676: goto st676;
		case 677: goto st677;
		case 678: goto st678;
		case 679: goto st679;
		case 680: goto st680;
		case 681: goto st681;
		case 783: goto st783;
		case 784: goto st784;
		case 785: goto st785;
		case 786: goto st786;
		case 682: goto st682;
		case 683: goto st683;
		case 684: goto st684;
		case 685: goto st685;
		case 686: goto st686;
		case 787: goto st787;
		case 788: goto st788;
		case 687: goto st687;
		case 688: goto st688;
		case 689: goto st689;
		case 690: goto st690;
		case 691: goto st691;
		case 692: goto st692;
		case 693: goto st693;
		case 694: goto st694;
		case 695: goto st695;
		case 696: goto st696;
		case 697: goto st697;
		case 698: goto st698;
		case 699: goto st699;
		case 700: goto st700;
		case 701: goto st701;
		case 702: goto st702;
		case 703: goto st703;
		case 704: goto st704;
		case 705: goto st705;
		case 706: goto st706;
		case 707: goto st707;
		case 708: goto st708;
		case 709: goto st709;
		case 710: goto st710;
		case 711: goto st711;
		case 712: goto st712;
	default: break;
	}

	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof;
_resume:
	switch (  sm->cs )
	{
tr0:
#line 1 "NONE"
	{	switch( ( sm->act) ) {
	case 109:
	{{( sm->p) = ((( sm->te)))-1;}
    g_debug("block newline2");

    if (header_mode) {
      dstack_close_leaf_blocks();
    } else if (dstack_is_open(BLOCK_UL)) {
      dstack_close_until(BLOCK_UL);
    } else {
      dstack_close_before_block();
    }
  }
	break;
	case 110:
	{{( sm->p) = ((( sm->te)))-1;}
    g_debug("block newline");
  }
	break;
	}
	}
	goto st713;
tr2:
#line 740 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{
    g_debug("block char: %c", (*( sm->p)));
    ( sm->p)--;

    if (dstack.empty() || dstack_check(BLOCK_QUOTE) || dstack_check(BLOCK_SPOILER) || dstack_check(BLOCK_SECTION)) {
      dstack_open_block(BLOCK_P, "<p>");
    }

    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 713;goto st729;}}
  }}
	goto st713;
tr16:
#line 648 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("block [/spoiler]");
    dstack_close_before_block();
    if (dstack_check( BLOCK_SPOILER)) {
      g_debug("  rewind");
      dstack_rewind();
    }
  }}
	goto st713;
tr47:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 631 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_QUOTE, "<blockquote class=\"dtext-quote-color\" style=\"border-left-color:");
    if(a1[0] == '#') {
      append("#");
      append_uri_escaped({ a1 + 1, a2 });
    } else {
      append_uri_escaped({ a1, a2 });
    }
    append("\">");
  }}
	goto st713;
tr53:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 624 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_QUOTE, "<blockquote class=\"dtext-sidebar-colored-");
    append_uri_escaped({ a1, a2 });
    append("\">");
  }}
	goto st713;
tr162:
#line 711 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_TABLE, "<table class=\"striped\">");
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 713;goto st787;}}
  }}
	goto st713;
tr784:
#line 740 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("block char: %c", (*( sm->p)));
    ( sm->p)--;

    if (dstack.empty() || dstack_check(BLOCK_QUOTE) || dstack_check(BLOCK_SPOILER) || dstack_check(BLOCK_SECTION)) {
      dstack_open_block(BLOCK_P, "<p>");
    }

    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 713;goto st729;}}
  }}
	goto st713;
tr791:
#line 601 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append("<span class=\"inline-code\">");
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 713;goto st783;}}
  }}
	goto st713;
tr793:
#line 740 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("block char: %c", (*( sm->p)));
    ( sm->p)--;

    if (dstack.empty() || dstack_check(BLOCK_QUOTE) || dstack_check(BLOCK_SPOILER) || dstack_check(BLOCK_SECTION)) {
      dstack_open_block(BLOCK_P, "<p>");
    }

    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 713;goto st729;}}
  }}
	goto st713;
tr794:
#line 96 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
#line 717 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("block list");
    dstack_open_list(a2 - a1);
    {( sm->p) = (( b1))-1;}
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 713;goto st729;}}
  }}
	goto st713;
tr797:
#line 606 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    static element_t blocks[] = { BLOCK_H1, BLOCK_H2, BLOCK_H3, BLOCK_H4, BLOCK_H5, BLOCK_H6 };
    char header = *a1;
    element_t block = blocks[header - '1'];

    dstack_open_block(block, "<h");
    append_block(header);
    append_block(">");

    header_mode = true;
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 713;goto st729;}}
  }}
	goto st713;
tr804:
#line 657 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_CODE, "<pre>");
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 713;goto st785;}}
  }}
	goto st713;
tr805:
#line 619 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_QUOTE, "<blockquote>");
  }}
	goto st713;
tr806:
#line 663 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    // Parse the optional attributes captured in a1/a2. The grammar marks the
    // attribute substring (if present) starting at a1 and ending at a2. It can
    // contain flags like "expanded" and/or an alias/summary after '='.
    bool initially_open = false;
    std::string_view summary_view;

    if (a1 && a2 && a2 > a1) {
      std::string_view attr(a1, a2 - a1);

      // trim leading/trailing whitespace
      size_t start = 0;
      while (start < attr.size() && std::isspace(static_cast<unsigned char>(attr[start]))) start++;
      size_t end = attr.size();
      while (end > start && std::isspace(static_cast<unsigned char>(attr[end - 1]))) end--;

      if (end > start) {
        attr = attr.substr(start, end - start);

        // look for 'expanded' flag (either alone or before/after '=')
        if (attr.find("expanded") != std::string_view::npos) {
          initially_open = true;
        }

        // if there's an '=', take the part after it as the summary; otherwise
        // if attr is not just 'expanded' then the whole attr is a summary
        size_t eq = attr.find('=');
        if (eq != std::string_view::npos) {
          summary_view = attr.substr(eq + 1);
          // trim summary
          size_t sstart = 0;
          while (sstart < summary_view.size() && std::isspace(static_cast<unsigned char>(summary_view[sstart]))) sstart++;
          size_t send = summary_view.size();
          while (send > sstart && std::isspace(static_cast<unsigned char>(summary_view[send - 1]))) send--;
          summary_view = summary_view.substr(sstart, send - sstart);
        } else if (attr != "expanded") {
          summary_view = attr;
        }
      }
    }

    if (!summary_view.empty()) {
      append_section({ summary_view.data(), summary_view.size() }, initially_open);
    } else {
      append_section({}, initially_open);
    }
  }}
	goto st713;
tr807:
#line 643 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    dstack_close_leaf_blocks();
    dstack_open_block(BLOCK_SPOILER, "<div class=\"spoiler\">");
  }}
	goto st713;
tr808:
#line 597 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append("`");
  }}
	goto st713;
st713:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof713;
case 713:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 1593 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr785;
		case 13: goto st715;
		case 42: goto tr787;
		case 72: goto tr788;
		case 91: goto tr789;
		case 92: goto st726;
		case 96: goto tr791;
		case 104: goto tr788;
	}
	goto tr784;
tr1:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 724 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 109;}
	goto st714;
tr785:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 736 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 110;}
	goto st714;
st714:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof714;
case 714:
#line 1616 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr1;
		case 13: goto st0;
	}
	goto tr0;
st0:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof0;
case 0:
	if ( (*( sm->p)) == 10 )
		goto tr1;
	goto tr0;
st715:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof715;
case 715:
	if ( (*( sm->p)) == 10 )
		goto tr785;
	goto tr793;
tr787:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st716;
st716:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof716;
case 716:
#line 1643 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr5;
		case 32: goto tr5;
		case 42: goto st2;
	}
	goto tr793;
tr5:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st1;
st1:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof1;
case 1:
#line 1656 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr4;
		case 10: goto tr2;
		case 13: goto tr2;
		case 32: goto tr4;
	}
	goto tr3;
tr3:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st717;
st717:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof717;
case 717:
#line 1670 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr794;
		case 13: goto tr794;
	}
	goto st717;
tr4:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st718;
st718:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof718;
case 718:
#line 1682 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr4;
		case 10: goto tr794;
		case 13: goto tr794;
		case 32: goto tr4;
	}
	goto tr3;
st2:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof2;
case 2:
	switch( (*( sm->p)) ) {
		case 9: goto tr5;
		case 32: goto tr5;
		case 42: goto st2;
	}
	goto tr2;
tr788:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st719;
st719:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof719;
case 719:
#line 1706 "ext/dtext/dtext.cpp"
	if ( 49 <= (*( sm->p)) && (*( sm->p)) <= 54 )
		goto tr796;
	goto tr793;
tr796:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st3;
st3:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof3;
case 3:
#line 1716 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 46 )
		goto tr7;
	goto tr2;
tr7:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st720;
st720:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof720;
case 720:
#line 1726 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto st720;
		case 32: goto st720;
	}
	goto tr797;
tr789:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st721;
st721:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof721;
case 721:
#line 1738 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 47: goto st4;
		case 67: goto st13;
		case 81: goto st17;
		case 83: goto st133;
		case 84: goto st148;
		case 99: goto st13;
		case 113: goto st17;
		case 115: goto st133;
		case 116: goto st148;
	}
	goto tr793;
st4:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof4;
case 4:
	switch( (*( sm->p)) ) {
		case 83: goto st5;
		case 115: goto st5;
	}
	goto tr2;
st5:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof5;
case 5:
	switch( (*( sm->p)) ) {
		case 80: goto st6;
		case 112: goto st6;
	}
	goto tr2;
st6:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof6;
case 6:
	switch( (*( sm->p)) ) {
		case 79: goto st7;
		case 111: goto st7;
	}
	goto tr2;
st7:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof7;
case 7:
	switch( (*( sm->p)) ) {
		case 73: goto st8;
		case 105: goto st8;
	}
	goto tr2;
st8:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof8;
case 8:
	switch( (*( sm->p)) ) {
		case 76: goto st9;
		case 108: goto st9;
	}
	goto tr2;
st9:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof9;
case 9:
	switch( (*( sm->p)) ) {
		case 69: goto st10;
		case 101: goto st10;
	}
	goto tr2;
st10:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof10;
case 10:
	switch( (*( sm->p)) ) {
		case 82: goto st11;
		case 114: goto st11;
	}
	goto tr2;
st11:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof11;
case 11:
	switch( (*( sm->p)) ) {
		case 83: goto st12;
		case 93: goto tr16;
		case 115: goto st12;
	}
	goto tr2;
st12:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof12;
case 12:
	if ( (*( sm->p)) == 93 )
		goto tr16;
	goto tr2;
st13:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof13;
case 13:
	switch( (*( sm->p)) ) {
		case 79: goto st14;
		case 111: goto st14;
	}
	goto tr2;
st14:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof14;
case 14:
	switch( (*( sm->p)) ) {
		case 68: goto st15;
		case 100: goto st15;
	}
	goto tr2;
st15:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof15;
case 15:
	switch( (*( sm->p)) ) {
		case 69: goto st16;
		case 101: goto st16;
	}
	goto tr2;
st16:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof16;
case 16:
	if ( (*( sm->p)) == 93 )
		goto st722;
	goto tr2;
st722:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof722;
case 722:
	if ( (*( sm->p)) == 32 )
		goto st722;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st722;
	goto tr804;
st17:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof17;
case 17:
	switch( (*( sm->p)) ) {
		case 85: goto st18;
		case 117: goto st18;
	}
	goto tr2;
st18:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof18;
case 18:
	switch( (*( sm->p)) ) {
		case 79: goto st19;
		case 111: goto st19;
	}
	goto tr2;
st19:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof19;
case 19:
	switch( (*( sm->p)) ) {
		case 84: goto st20;
		case 116: goto st20;
	}
	goto tr2;
st20:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof20;
case 20:
	switch( (*( sm->p)) ) {
		case 69: goto st21;
		case 101: goto st21;
	}
	goto tr2;
st21:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof21;
case 21:
	switch( (*( sm->p)) ) {
		case 61: goto st22;
		case 93: goto st723;
	}
	goto tr2;
st22:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof22;
case 22:
	switch( (*( sm->p)) ) {
		case 35: goto tr27;
		case 65: goto tr28;
		case 67: goto tr29;
		case 71: goto tr30;
		case 73: goto tr31;
		case 76: goto tr32;
		case 77: goto tr33;
		case 83: goto tr34;
		case 97: goto tr35;
		case 99: goto tr37;
		case 103: goto tr38;
		case 105: goto tr39;
		case 108: goto tr40;
		case 109: goto tr41;
		case 115: goto tr42;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto tr36;
	goto tr2;
tr27:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st23;
st23:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof23;
case 23:
#line 1949 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st24;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st24;
	} else
		goto st24;
	goto tr2;
st24:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof24;
case 24:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st25;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st25;
	} else
		goto st25;
	goto tr2;
st25:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof25;
case 25:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st26;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st26;
	} else
		goto st26;
	goto tr2;
st26:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof26;
case 26:
	if ( (*( sm->p)) == 93 )
		goto tr47;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st27;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st27;
	} else
		goto st27;
	goto tr2;
st27:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof27;
case 27:
	if ( (*( sm->p)) == 93 )
		goto tr47;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st28;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st28;
	} else
		goto st28;
	goto tr2;
st28:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof28;
case 28:
	if ( (*( sm->p)) == 93 )
		goto tr47;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st29;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st29;
	} else
		goto st29;
	goto tr2;
st29:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof29;
case 29:
	if ( (*( sm->p)) == 93 )
		goto tr47;
	goto tr2;
tr28:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st30;
st30:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof30;
case 30:
#line 2043 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st31;
		case 114: goto st31;
	}
	goto tr2;
st31:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof31;
case 31:
	switch( (*( sm->p)) ) {
		case 84: goto st32;
		case 116: goto st32;
	}
	goto tr2;
st32:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof32;
case 32:
	switch( (*( sm->p)) ) {
		case 73: goto st33;
		case 93: goto tr53;
		case 105: goto st33;
	}
	goto tr2;
st33:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof33;
case 33:
	switch( (*( sm->p)) ) {
		case 83: goto st34;
		case 115: goto st34;
	}
	goto tr2;
st34:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof34;
case 34:
	switch( (*( sm->p)) ) {
		case 84: goto st35;
		case 116: goto st35;
	}
	goto tr2;
st35:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof35;
case 35:
	if ( (*( sm->p)) == 93 )
		goto tr53;
	goto tr2;
tr29:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st36;
st36:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof36;
case 36:
#line 2099 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st37;
		case 79: goto st44;
		case 104: goto st37;
		case 111: goto st44;
	}
	goto tr2;
st37:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof37;
case 37:
	switch( (*( sm->p)) ) {
		case 65: goto st38;
		case 97: goto st38;
	}
	goto tr2;
st38:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof38;
case 38:
	switch( (*( sm->p)) ) {
		case 82: goto st39;
		case 114: goto st39;
	}
	goto tr2;
st39:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof39;
case 39:
	switch( (*( sm->p)) ) {
		case 65: goto st40;
		case 93: goto tr53;
		case 97: goto st40;
	}
	goto tr2;
st40:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof40;
case 40:
	switch( (*( sm->p)) ) {
		case 67: goto st41;
		case 99: goto st41;
	}
	goto tr2;
st41:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof41;
case 41:
	switch( (*( sm->p)) ) {
		case 84: goto st42;
		case 116: goto st42;
	}
	goto tr2;
st42:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof42;
case 42:
	switch( (*( sm->p)) ) {
		case 69: goto st43;
		case 101: goto st43;
	}
	goto tr2;
st43:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof43;
case 43:
	switch( (*( sm->p)) ) {
		case 82: goto st35;
		case 114: goto st35;
	}
	goto tr2;
st44:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof44;
case 44:
	switch( (*( sm->p)) ) {
		case 78: goto st45;
		case 80: goto st52;
		case 110: goto st45;
		case 112: goto st52;
	}
	goto tr2;
st45:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof45;
case 45:
	switch( (*( sm->p)) ) {
		case 84: goto st46;
		case 116: goto st46;
	}
	goto tr2;
st46:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof46;
case 46:
	switch( (*( sm->p)) ) {
		case 82: goto st47;
		case 93: goto tr53;
		case 114: goto st47;
	}
	goto tr2;
st47:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof47;
case 47:
	switch( (*( sm->p)) ) {
		case 73: goto st48;
		case 105: goto st48;
	}
	goto tr2;
st48:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof48;
case 48:
	switch( (*( sm->p)) ) {
		case 66: goto st49;
		case 98: goto st49;
	}
	goto tr2;
st49:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof49;
case 49:
	switch( (*( sm->p)) ) {
		case 85: goto st50;
		case 117: goto st50;
	}
	goto tr2;
st50:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof50;
case 50:
	switch( (*( sm->p)) ) {
		case 84: goto st51;
		case 116: goto st51;
	}
	goto tr2;
st51:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof51;
case 51:
	switch( (*( sm->p)) ) {
		case 79: goto st43;
		case 111: goto st43;
	}
	goto tr2;
st52:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof52;
case 52:
	switch( (*( sm->p)) ) {
		case 89: goto st53;
		case 121: goto st53;
	}
	goto tr2;
st53:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof53;
case 53:
	switch( (*( sm->p)) ) {
		case 82: goto st54;
		case 93: goto tr53;
		case 114: goto st54;
	}
	goto tr2;
st54:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof54;
case 54:
	switch( (*( sm->p)) ) {
		case 73: goto st55;
		case 105: goto st55;
	}
	goto tr2;
st55:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof55;
case 55:
	switch( (*( sm->p)) ) {
		case 71: goto st56;
		case 103: goto st56;
	}
	goto tr2;
st56:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof56;
case 56:
	switch( (*( sm->p)) ) {
		case 72: goto st34;
		case 104: goto st34;
	}
	goto tr2;
tr30:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st57;
st57:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof57;
case 57:
#line 2298 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st58;
		case 101: goto st58;
	}
	goto tr2;
st58:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof58;
case 58:
	switch( (*( sm->p)) ) {
		case 78: goto st59;
		case 110: goto st59;
	}
	goto tr2;
st59:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof59;
case 59:
	switch( (*( sm->p)) ) {
		case 69: goto st60;
		case 93: goto tr53;
		case 101: goto st60;
	}
	goto tr2;
st60:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof60;
case 60:
	switch( (*( sm->p)) ) {
		case 82: goto st61;
		case 114: goto st61;
	}
	goto tr2;
st61:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof61;
case 61:
	switch( (*( sm->p)) ) {
		case 65: goto st62;
		case 97: goto st62;
	}
	goto tr2;
st62:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof62;
case 62:
	switch( (*( sm->p)) ) {
		case 76: goto st35;
		case 108: goto st35;
	}
	goto tr2;
tr31:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st63;
st63:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof63;
case 63:
#line 2356 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st64;
		case 110: goto st64;
	}
	goto tr2;
st64:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof64;
case 64:
	switch( (*( sm->p)) ) {
		case 86: goto st65;
		case 118: goto st65;
	}
	goto tr2;
st65:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof65;
case 65:
	switch( (*( sm->p)) ) {
		case 65: goto st66;
		case 93: goto tr53;
		case 97: goto st66;
	}
	goto tr2;
st66:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof66;
case 66:
	switch( (*( sm->p)) ) {
		case 76: goto st67;
		case 108: goto st67;
	}
	goto tr2;
st67:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof67;
case 67:
	switch( (*( sm->p)) ) {
		case 73: goto st68;
		case 105: goto st68;
	}
	goto tr2;
st68:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof68;
case 68:
	switch( (*( sm->p)) ) {
		case 68: goto st35;
		case 100: goto st35;
	}
	goto tr2;
tr32:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st69;
st69:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof69;
case 69:
#line 2414 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st70;
		case 111: goto st70;
	}
	goto tr2;
st70:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof70;
case 70:
	switch( (*( sm->p)) ) {
		case 82: goto st71;
		case 114: goto st71;
	}
	goto tr2;
st71:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof71;
case 71:
	switch( (*( sm->p)) ) {
		case 69: goto st35;
		case 93: goto tr53;
		case 101: goto st35;
	}
	goto tr2;
tr33:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st72;
st72:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof72;
case 72:
#line 2445 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st73;
		case 101: goto st73;
	}
	goto tr2;
st73:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof73;
case 73:
	switch( (*( sm->p)) ) {
		case 84: goto st74;
		case 116: goto st74;
	}
	goto tr2;
st74:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof74;
case 74:
	switch( (*( sm->p)) ) {
		case 65: goto st35;
		case 97: goto st35;
	}
	goto tr2;
tr34:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st75;
st75:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof75;
case 75:
#line 2475 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st76;
		case 112: goto st76;
	}
	goto tr2;
st76:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof76;
case 76:
	switch( (*( sm->p)) ) {
		case 69: goto st77;
		case 101: goto st77;
	}
	goto tr2;
st77:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof77;
case 77:
	switch( (*( sm->p)) ) {
		case 67: goto st78;
		case 99: goto st78;
	}
	goto tr2;
st78:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof78;
case 78:
	switch( (*( sm->p)) ) {
		case 73: goto st79;
		case 93: goto tr53;
		case 105: goto st79;
	}
	goto tr2;
st79:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof79;
case 79:
	switch( (*( sm->p)) ) {
		case 69: goto st80;
		case 101: goto st80;
	}
	goto tr2;
st80:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof80;
case 80:
	switch( (*( sm->p)) ) {
		case 83: goto st35;
		case 115: goto st35;
	}
	goto tr2;
tr35:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st81;
st81:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof81;
case 81:
#line 2533 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st31;
		case 93: goto tr47;
		case 114: goto st83;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr36:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st82;
st82:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof82;
case 82:
#line 2548 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr47;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st83:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof83;
case 83:
	switch( (*( sm->p)) ) {
		case 84: goto st32;
		case 93: goto tr47;
		case 116: goto st84;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st84:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof84;
case 84:
	switch( (*( sm->p)) ) {
		case 73: goto st33;
		case 93: goto tr53;
		case 105: goto st85;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st85:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof85;
case 85:
	switch( (*( sm->p)) ) {
		case 83: goto st34;
		case 93: goto tr47;
		case 115: goto st86;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st86:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof86;
case 86:
	switch( (*( sm->p)) ) {
		case 84: goto st35;
		case 93: goto tr47;
		case 116: goto st87;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st87:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof87;
case 87:
	if ( (*( sm->p)) == 93 )
		goto tr53;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr37:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st88;
st88:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof88;
case 88:
#line 2617 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st37;
		case 79: goto st44;
		case 93: goto tr47;
		case 104: goto st89;
		case 111: goto st96;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st89:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof89;
case 89:
	switch( (*( sm->p)) ) {
		case 65: goto st38;
		case 93: goto tr47;
		case 97: goto st90;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st90:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof90;
case 90:
	switch( (*( sm->p)) ) {
		case 82: goto st39;
		case 93: goto tr47;
		case 114: goto st91;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st91:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof91;
case 91:
	switch( (*( sm->p)) ) {
		case 65: goto st40;
		case 93: goto tr53;
		case 97: goto st92;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st92:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof92;
case 92:
	switch( (*( sm->p)) ) {
		case 67: goto st41;
		case 93: goto tr47;
		case 99: goto st93;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st93:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof93;
case 93:
	switch( (*( sm->p)) ) {
		case 84: goto st42;
		case 93: goto tr47;
		case 116: goto st94;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st94:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof94;
case 94:
	switch( (*( sm->p)) ) {
		case 69: goto st43;
		case 93: goto tr47;
		case 101: goto st95;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st95:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof95;
case 95:
	switch( (*( sm->p)) ) {
		case 82: goto st35;
		case 93: goto tr47;
		case 114: goto st87;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st96:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof96;
case 96:
	switch( (*( sm->p)) ) {
		case 78: goto st45;
		case 80: goto st52;
		case 93: goto tr47;
		case 110: goto st97;
		case 112: goto st104;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st97:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof97;
case 97:
	switch( (*( sm->p)) ) {
		case 84: goto st46;
		case 93: goto tr47;
		case 116: goto st98;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st98:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof98;
case 98:
	switch( (*( sm->p)) ) {
		case 82: goto st47;
		case 93: goto tr53;
		case 114: goto st99;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st99:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof99;
case 99:
	switch( (*( sm->p)) ) {
		case 73: goto st48;
		case 93: goto tr47;
		case 105: goto st100;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st100:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof100;
case 100:
	switch( (*( sm->p)) ) {
		case 66: goto st49;
		case 93: goto tr47;
		case 98: goto st101;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st101:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof101;
case 101:
	switch( (*( sm->p)) ) {
		case 85: goto st50;
		case 93: goto tr47;
		case 117: goto st102;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st102:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof102;
case 102:
	switch( (*( sm->p)) ) {
		case 84: goto st51;
		case 93: goto tr47;
		case 116: goto st103;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st103:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof103;
case 103:
	switch( (*( sm->p)) ) {
		case 79: goto st43;
		case 93: goto tr47;
		case 111: goto st95;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st104:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof104;
case 104:
	switch( (*( sm->p)) ) {
		case 89: goto st53;
		case 93: goto tr47;
		case 121: goto st105;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st105:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof105;
case 105:
	switch( (*( sm->p)) ) {
		case 82: goto st54;
		case 93: goto tr53;
		case 114: goto st106;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st106:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof106;
case 106:
	switch( (*( sm->p)) ) {
		case 73: goto st55;
		case 93: goto tr47;
		case 105: goto st107;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st107:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof107;
case 107:
	switch( (*( sm->p)) ) {
		case 71: goto st56;
		case 93: goto tr47;
		case 103: goto st108;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st108:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof108;
case 108:
	switch( (*( sm->p)) ) {
		case 72: goto st34;
		case 93: goto tr47;
		case 104: goto st86;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr38:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st109;
st109:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof109;
case 109:
#line 2876 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st58;
		case 93: goto tr47;
		case 101: goto st110;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st110:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof110;
case 110:
	switch( (*( sm->p)) ) {
		case 78: goto st59;
		case 93: goto tr47;
		case 110: goto st111;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st111:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof111;
case 111:
	switch( (*( sm->p)) ) {
		case 69: goto st60;
		case 93: goto tr53;
		case 101: goto st112;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st112:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof112;
case 112:
	switch( (*( sm->p)) ) {
		case 82: goto st61;
		case 93: goto tr47;
		case 114: goto st113;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st113:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof113;
case 113:
	switch( (*( sm->p)) ) {
		case 65: goto st62;
		case 93: goto tr47;
		case 97: goto st114;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st114:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof114;
case 114:
	switch( (*( sm->p)) ) {
		case 76: goto st35;
		case 93: goto tr47;
		case 108: goto st87;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr39:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st115;
st115:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof115;
case 115:
#line 2951 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st64;
		case 93: goto tr47;
		case 110: goto st116;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st116:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof116;
case 116:
	switch( (*( sm->p)) ) {
		case 86: goto st65;
		case 93: goto tr47;
		case 118: goto st117;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st117:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof117;
case 117:
	switch( (*( sm->p)) ) {
		case 65: goto st66;
		case 93: goto tr53;
		case 97: goto st118;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st118:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof118;
case 118:
	switch( (*( sm->p)) ) {
		case 76: goto st67;
		case 93: goto tr47;
		case 108: goto st119;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st119:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof119;
case 119:
	switch( (*( sm->p)) ) {
		case 73: goto st68;
		case 93: goto tr47;
		case 105: goto st120;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st120:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof120;
case 120:
	switch( (*( sm->p)) ) {
		case 68: goto st35;
		case 93: goto tr47;
		case 100: goto st87;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr40:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st121;
st121:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof121;
case 121:
#line 3026 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st70;
		case 93: goto tr47;
		case 111: goto st122;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st122:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof122;
case 122:
	switch( (*( sm->p)) ) {
		case 82: goto st71;
		case 93: goto tr47;
		case 114: goto st123;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st123:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof123;
case 123:
	switch( (*( sm->p)) ) {
		case 69: goto st35;
		case 93: goto tr53;
		case 101: goto st87;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr41:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st124;
st124:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof124;
case 124:
#line 3065 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st73;
		case 93: goto tr47;
		case 101: goto st125;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st125:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof125;
case 125:
	switch( (*( sm->p)) ) {
		case 84: goto st74;
		case 93: goto tr47;
		case 116: goto st126;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st126:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof126;
case 126:
	switch( (*( sm->p)) ) {
		case 65: goto st35;
		case 93: goto tr47;
		case 97: goto st87;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
tr42:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st127;
st127:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof127;
case 127:
#line 3104 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st76;
		case 93: goto tr47;
		case 112: goto st128;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st128:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof128;
case 128:
	switch( (*( sm->p)) ) {
		case 69: goto st77;
		case 93: goto tr47;
		case 101: goto st129;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st129:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof129;
case 129:
	switch( (*( sm->p)) ) {
		case 67: goto st78;
		case 93: goto tr47;
		case 99: goto st130;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st130:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof130;
case 130:
	switch( (*( sm->p)) ) {
		case 73: goto st79;
		case 93: goto tr53;
		case 105: goto st131;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st131:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof131;
case 131:
	switch( (*( sm->p)) ) {
		case 69: goto st80;
		case 93: goto tr47;
		case 101: goto st132;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st132:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof132;
case 132:
	switch( (*( sm->p)) ) {
		case 83: goto st35;
		case 93: goto tr47;
		case 115: goto st87;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st82;
	goto tr2;
st723:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof723;
case 723:
	if ( (*( sm->p)) == 32 )
		goto st723;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st723;
	goto tr805;
st133:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof133;
case 133:
	switch( (*( sm->p)) ) {
		case 69: goto st134;
		case 80: goto st141;
		case 101: goto st134;
		case 112: goto st141;
	}
	goto tr2;
st134:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof134;
case 134:
	switch( (*( sm->p)) ) {
		case 67: goto st135;
		case 99: goto st135;
	}
	goto tr2;
st135:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof135;
case 135:
	switch( (*( sm->p)) ) {
		case 84: goto st136;
		case 116: goto st136;
	}
	goto tr2;
st136:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof136;
case 136:
	switch( (*( sm->p)) ) {
		case 73: goto st137;
		case 105: goto st137;
	}
	goto tr2;
st137:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof137;
case 137:
	switch( (*( sm->p)) ) {
		case 79: goto st138;
		case 111: goto st138;
	}
	goto tr2;
st138:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof138;
case 138:
	switch( (*( sm->p)) ) {
		case 78: goto st139;
		case 110: goto st139;
	}
	goto tr2;
st139:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof139;
case 139:
	switch( (*( sm->p)) ) {
		case 44: goto tr147;
		case 61: goto tr147;
		case 93: goto st724;
	}
	goto tr2;
tr147:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st140;
st140:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof140;
case 140:
#line 3254 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr150;
	goto st140;
tr150:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st724;
st724:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof724;
case 724:
#line 3264 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 32 )
		goto st724;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st724;
	goto tr806;
st141:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof141;
case 141:
	switch( (*( sm->p)) ) {
		case 79: goto st142;
		case 111: goto st142;
	}
	goto tr2;
st142:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof142;
case 142:
	switch( (*( sm->p)) ) {
		case 73: goto st143;
		case 105: goto st143;
	}
	goto tr2;
st143:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof143;
case 143:
	switch( (*( sm->p)) ) {
		case 76: goto st144;
		case 108: goto st144;
	}
	goto tr2;
st144:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof144;
case 144:
	switch( (*( sm->p)) ) {
		case 69: goto st145;
		case 101: goto st145;
	}
	goto tr2;
st145:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof145;
case 145:
	switch( (*( sm->p)) ) {
		case 82: goto st146;
		case 114: goto st146;
	}
	goto tr2;
st146:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof146;
case 146:
	switch( (*( sm->p)) ) {
		case 83: goto st147;
		case 93: goto st725;
		case 115: goto st147;
	}
	goto tr2;
st147:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof147;
case 147:
	if ( (*( sm->p)) == 93 )
		goto st725;
	goto tr2;
st725:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof725;
case 725:
	if ( (*( sm->p)) == 32 )
		goto st725;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st725;
	goto tr807;
st148:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof148;
case 148:
	switch( (*( sm->p)) ) {
		case 65: goto st149;
		case 97: goto st149;
	}
	goto tr2;
st149:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof149;
case 149:
	switch( (*( sm->p)) ) {
		case 66: goto st150;
		case 98: goto st150;
	}
	goto tr2;
st150:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof150;
case 150:
	switch( (*( sm->p)) ) {
		case 76: goto st151;
		case 108: goto st151;
	}
	goto tr2;
st151:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof151;
case 151:
	switch( (*( sm->p)) ) {
		case 69: goto st152;
		case 101: goto st152;
	}
	goto tr2;
st152:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof152;
case 152:
	if ( (*( sm->p)) == 93 )
		goto tr162;
	goto tr2;
st726:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof726;
case 726:
	if ( (*( sm->p)) == 96 )
		goto tr808;
	goto tr793;
tr163:
#line 208 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{ append_html_escaped((*( sm->p))); }}
	goto st727;
tr168:
#line 197 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_B, "</strong>"); }}
	goto st727;
tr169:
#line 199 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_I, "</em>"); }}
	goto st727;
tr171:
#line 201 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_S, "</s>"); }}
	goto st727;
tr174:
#line 207 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_SUB, "</sub>"); }}
	goto st727;
tr175:
#line 205 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_SUP, "</sup>"); }}
	goto st727;
tr176:
#line 203 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_U, "</u>"); }}
	goto st727;
tr177:
#line 196 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_B, "<strong>"); }}
	goto st727;
tr178:
#line 198 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_I, "<em>"); }}
	goto st727;
tr180:
#line 200 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_S, "<s>"); }}
	goto st727;
tr183:
#line 206 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_SUB, "<sub>"); }}
	goto st727;
tr184:
#line 204 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_SUP, "<sup>"); }}
	goto st727;
tr185:
#line 202 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_U, "<u>"); }}
	goto st727;
tr809:
#line 208 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ append_html_escaped((*( sm->p))); }}
	goto st727;
tr811:
#line 208 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_html_escaped((*( sm->p))); }}
	goto st727;
st727:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof727;
case 727:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 3441 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 91 )
		goto tr810;
	goto tr809;
tr810:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st728;
st728:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof728;
case 728:
#line 3451 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 47: goto st153;
		case 66: goto st161;
		case 73: goto st162;
		case 83: goto st163;
		case 85: goto st167;
		case 98: goto st161;
		case 105: goto st162;
		case 115: goto st163;
		case 117: goto st167;
	}
	goto tr811;
st153:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof153;
case 153:
	switch( (*( sm->p)) ) {
		case 66: goto st154;
		case 73: goto st155;
		case 83: goto st156;
		case 85: goto st160;
		case 98: goto st154;
		case 105: goto st155;
		case 115: goto st156;
		case 117: goto st160;
	}
	goto tr163;
st154:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof154;
case 154:
	if ( (*( sm->p)) == 93 )
		goto tr168;
	goto tr163;
st155:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof155;
case 155:
	if ( (*( sm->p)) == 93 )
		goto tr169;
	goto tr163;
st156:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof156;
case 156:
	switch( (*( sm->p)) ) {
		case 85: goto st157;
		case 93: goto tr171;
		case 117: goto st157;
	}
	goto tr163;
st157:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof157;
case 157:
	switch( (*( sm->p)) ) {
		case 66: goto st158;
		case 80: goto st159;
		case 98: goto st158;
		case 112: goto st159;
	}
	goto tr163;
st158:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof158;
case 158:
	if ( (*( sm->p)) == 93 )
		goto tr174;
	goto tr163;
st159:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof159;
case 159:
	if ( (*( sm->p)) == 93 )
		goto tr175;
	goto tr163;
st160:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof160;
case 160:
	if ( (*( sm->p)) == 93 )
		goto tr176;
	goto tr163;
st161:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof161;
case 161:
	if ( (*( sm->p)) == 93 )
		goto tr177;
	goto tr163;
st162:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof162;
case 162:
	if ( (*( sm->p)) == 93 )
		goto tr178;
	goto tr163;
st163:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof163;
case 163:
	switch( (*( sm->p)) ) {
		case 85: goto st164;
		case 93: goto tr180;
		case 117: goto st164;
	}
	goto tr163;
st164:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof164;
case 164:
	switch( (*( sm->p)) ) {
		case 66: goto st165;
		case 80: goto st166;
		case 98: goto st165;
		case 112: goto st166;
	}
	goto tr163;
st165:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof165;
case 165:
	if ( (*( sm->p)) == 93 )
		goto tr183;
	goto tr163;
st166:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof166;
case 166:
	if ( (*( sm->p)) == 93 )
		goto tr184;
	goto tr163;
st167:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof167;
case 167:
	if ( (*( sm->p)) == 93 )
		goto tr185;
	goto tr163;
tr186:
#line 1 "NONE"
	{	switch( ( sm->act) ) {
	case 78:
	{{( sm->p) = ((( sm->te)))-1;}
    g_debug("inline newline2");
    g_debug("  return");

    dstack_close_list();

    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }
	break;
	case 79:
	{{( sm->p) = ((( sm->te)))-1;}
    g_debug("inline newline");

    if (header_mode) {
      dstack_close_leaf_blocks();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else if (dstack_is_open(BLOCK_UL)) {
      dstack_close_list();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else {
      append("<br>");
    }
  }
	break;
	case 81:
	{{( sm->p) = ((( sm->te)))-1;}
    g_debug("inline char: %c", (*( sm->p)));
    append_html_escaped((*( sm->p)));
  }
	break;
	}
	}
	goto st729;
tr188:
#line 488 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{
    g_debug("inline newline2");
    g_debug("  return");

    dstack_close_list();

    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st729;
tr199:
#line 372 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [/spoiler]");
    dstack_close_before_block();

    if (dstack_check(INLINE_SPOILER)) {
      dstack_close_inline(INLINE_SPOILER, "</span>");
    } else if (dstack_close_block(BLOCK_SPOILER, "</div>")) {
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    }
  }}
	goto st729;
tr201:
#line 482 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_close_block(BLOCK_TD, "</td>")) {
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    }
  }}
	goto st729;
tr202:
#line 498 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{
    g_debug("inline newline");

    if (header_mode) {
      dstack_close_leaf_blocks();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else if (dstack_is_open(BLOCK_UL)) {
      dstack_close_list();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else {
      append("<br>");
    }
  }}
	goto st729;
tr224:
#line 516 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{
    g_debug("inline char: %c", (*( sm->p)));
    append_html_escaped((*( sm->p)));
  }}
	goto st729;
tr242:
#line 96 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
#line 297 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_named_url({ b1, b2 }, { a1, a2 });
  }}
	goto st729;
tr258:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 313 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_unnamed_url({ a1, a2 });
  }}
	goto st729;
tr419:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 221 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append("<a id=\"");
    std::string lowercased_tag = std::string(a1, a2 - a1);
    std::transform(lowercased_tag.begin(), lowercased_tag.end(), lowercased_tag.begin(), [](unsigned char c) { return std::tolower(c); });
    append_uri_escaped(lowercased_tag);
    append("\"></a>");
  }}
	goto st729;
tr426:
#line 324 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_B, "</strong>"); }}
	goto st729;
tr434:
#line 361 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if(options.allow_color) {
      dstack_close_inline(INLINE_COLOR, "</span>");
    }
    {goto st729;}
  }}
	goto st729;
tr435:
#line 326 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_I, "</em>"); }}
	goto st729;
tr437:
#line 328 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_S, "</s>"); }}
	goto st729;
tr440:
#line 334 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_SUB, "</sub>"); }}
	goto st729;
tr441:
#line 332 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_SUP, "</sup>"); }}
	goto st729;
tr448:
#line 476 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_close_block(BLOCK_TH, "</th>")) {
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    }
  }}
	goto st729;
tr449:
#line 330 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_close_inline(INLINE_U, "</u>"); }}
	goto st729;
tr450:
#line 323 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_B, "<strong>"); }}
	goto st729;
tr455:
#line 414 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_before_block();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st729;
tr479:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 346 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if(options.allow_color) {
      dstack_push(INLINE_COLOR);
      append("<span class=\"dtext-color\" style=\"color:");
      if(a1[0] == '#') {
        append("#");
        append_uri_escaped({ a1 + 1, a2 });
      } else {
        append_uri_escaped({ a1, a2 });
      }
      append("\">");
    }
    {goto st729;}
  }}
	goto st729;
tr485:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 336 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if(options.allow_color) {
      dstack_push(INLINE_COLOR);
      append("<span class=\"dtext-color-");
      append_uri_escaped({ a1, a2 });
      append("\">");
    }
    {goto st729;}
  }}
	goto st729;
tr572:
#line 325 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_I, "<em>"); }}
	goto st729;
tr578:
#line 436 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [quote]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st729;
tr599:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 443 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [quote=color]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st729;
tr605:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 450 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [quote=type]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st729;
tr695:
#line 327 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_S, "<s>"); }}
	goto st729;
tr702:
#line 463 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [section]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st729;
tr704:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 463 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline [section]");
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st729;
tr711:
#line 368 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_inline(INLINE_SPOILER, "<span class=\"spoiler\">");
  }}
	goto st729;
tr714:
#line 333 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_SUB, "<sub>"); }}
	goto st729;
tr715:
#line 331 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_SUP, "<sup>"); }}
	goto st729;
tr720:
#line 392 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_before_block();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st729;
tr721:
#line 329 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{ dstack_open_inline(INLINE_U, "<u>"); }}
	goto st729;
tr727:
#line 277 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_wiki_link({ a1, a2 }, { a1, a2 });
  }}
	goto st729;
tr731:
#line 281 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_wiki_link({ a1, a2 }, { b1, b2 });
  }}
	goto st729;
tr741:
#line 273 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_post_search_link({ a1, a2 }, { b1, b2 });
  }}
	goto st729;
tr742:
#line 269 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_post_search_link({ a1, a2 }, { a1, a2 });
  }}
	goto st729;
tr817:
#line 516 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    g_debug("inline char: %c", (*( sm->p)));
    append_html_escaped((*( sm->p)));
  }}
	goto st729;
tr838:
#line 216 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append("<span class=\"inline-code\">");
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 729;goto st783;}}
  }}
	goto st729;
tr840:
#line 498 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline newline");

    if (header_mode) {
      dstack_close_leaf_blocks();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else if (dstack_is_open(BLOCK_UL)) {
      dstack_close_list();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else {
      append("<br>");
    }
  }}
	goto st729;
tr845:
#line 488 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline newline2");
    g_debug("  return");

    dstack_close_list();

    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st729;
tr847:
#line 96 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
#line 317 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline list");
    {( sm->p) = (( ts + 1))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st729;
tr849:
#line 386 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    dstack_close_leaf_blocks();
    {( sm->p) = (( ts))-1;}
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st729;
tr851:
#line 457 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline [/quote]");
    dstack_close_until(BLOCK_QUOTE);
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st729;
tr852:
#line 470 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline [/expand]");
    dstack_close_until(BLOCK_SECTION);
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st729;
tr853:
#line 512 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    append(' ');
  }}
	goto st729;
tr854:
#line 516 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline char: %c", (*( sm->p)));
    append_html_escaped((*( sm->p)));
  }}
	goto st729;
tr856:
#line 96 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
#line 285 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    const char* match_end = b2;
    const char* url_start = b1;
    const char* url_end = find_boundary_c(match_end - 1) + 1;

    append_named_url({ url_start, url_end }, { a1, a2 });

    if (url_end < match_end) {
      append_html_escaped({ url_end, match_end });
    }
  }}
	goto st729;
tr860:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 259 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("alias", "tag-alias", "/tag_aliases/"); }}
	goto st729;
tr862:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 256 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("artist", "artist", "/artists/"); }}
	goto st729;
tr867:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 257 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("ban", "ban", "/bans/"); }}
	goto st729;
tr869:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 265 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("blip", "blip", "/blips/"); }}
	goto st729;
tr871:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 258 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("BUR", "bulk-update-request", "/bulk_update_requests/"); }}
	goto st729;
tr874:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 253 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("comment", "comment", "/comments/"); }}
	goto st729;
tr878:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 249 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("flag", "post-flag", "/post_flags/"); }}
	goto st729;
tr880:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 251 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("forum", "forum-post", "/forum_posts/"); }}
	goto st729;
tr883:
#line 301 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    const char* match_end = te;
    const char* url_start = ts;
    const char* url_end = find_boundary_c(match_end - 1) + 1;

    append_unnamed_url({ url_start, url_end });

    if (url_end < match_end) {
      append_html_escaped({ url_end, match_end });
    }
  }}
	goto st729;
tr885:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 260 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("implication", "tag-implication", "/tag_implications/"); }}
	goto st729;
tr888:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 261 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("mod action", "mod-action", "/mod_actions/"); }}
	goto st729;
tr891:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 250 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("note", "note", "/notes/"); }}
	goto st729;
tr894:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 254 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("pool", "pool", "/pools/"); }}
	goto st729;
tr896:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 247 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("post", "post", "/posts/"); }}
	goto st729;
tr898:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 248 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("post changes", "post-changes-for", "/post_versions?search[post_id]="); }}
	goto st729;
tr901:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 262 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("record", "user-feedback", "/user_feedbacks/"); }}
	goto st729;
tr904:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 264 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("set", "set", "/post_sets/"); }}
	goto st729;
tr910:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 267 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("takedown", "takedown", "/takedowns/"); }}
	goto st729;
tr912:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 229 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    if(posts.size() < options.max_thumbs) {
      long post_id = strtol(a1, (char**)&a2, 10);
      posts.push_back(post_id);
      append("<a class=\"dtext-link dtext-id-link dtext-post-id-link thumb-placeholder-link\" data-id=\"");
      append_html_escaped({ a1, a2 });
      append("\" href=\"");
      append_url("/posts/");
      append_uri_escaped({ a1, a2 });
      append("\">");
      append("post #");
      append_html_escaped({ a1, a2 });
      append("</a>");
    } else {
      append_id_link("post", "post", "/posts/");
    }
  }}
	goto st729;
tr914:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 266 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("ticket", "ticket", "/tickets/"); }}
	goto st729;
tr916:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 252 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("topic", "forum-topic", "/forum_topics/"); }}
	goto st729;
tr919:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 255 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("user", "user", "/users/"); }}
	goto st729;
tr922:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
#line 263 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{ append_id_link("wiki", "wiki-page", "/wiki_pages/"); }}
	goto st729;
tr934:
#line 420 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline [/code]");
    dstack_close_before_block();

    if (dstack_check(BLOCK_LI)) {
      dstack_close_list();
    }

    if (dstack_check(BLOCK_CODE)) {
      dstack_rewind();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else {
      append_block("[/code]");
    }
  }}
	goto st729;
tr935:
#line 398 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    g_debug("inline [/table]");
    dstack_close_before_block();

    if (dstack_check(BLOCK_LI)) {
      dstack_close_list();
    }

    if (dstack_check(BLOCK_TABLE)) {
      dstack_rewind();
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    } else {
      append_block("[/table]");
    }
  }}
	goto st729;
tr936:
#line 212 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append("`");
  }}
	goto st729;
st729:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof729;
case 729:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 4128 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr818;
		case 13: goto st737;
		case 34: goto tr820;
		case 60: goto tr821;
		case 65: goto tr822;
		case 66: goto tr823;
		case 67: goto tr824;
		case 70: goto tr825;
		case 72: goto tr826;
		case 73: goto tr827;
		case 77: goto tr828;
		case 78: goto tr829;
		case 80: goto tr830;
		case 82: goto tr831;
		case 83: goto tr832;
		case 84: goto tr833;
		case 85: goto tr834;
		case 87: goto tr835;
		case 91: goto tr836;
		case 92: goto st781;
		case 96: goto tr838;
		case 97: goto tr822;
		case 98: goto tr823;
		case 99: goto tr824;
		case 102: goto tr825;
		case 104: goto tr826;
		case 105: goto tr827;
		case 109: goto tr828;
		case 110: goto tr829;
		case 112: goto tr830;
		case 114: goto tr831;
		case 115: goto tr832;
		case 116: goto tr833;
		case 117: goto tr834;
		case 119: goto tr835;
		case 123: goto tr839;
	}
	goto tr817;
tr818:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 498 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 79;}
	goto st730;
st730:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof730;
case 730:
#line 4175 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr187;
		case 13: goto st168;
		case 42: goto tr842;
		case 72: goto st183;
		case 91: goto st185;
		case 104: goto st183;
	}
	goto tr840;
tr187:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 488 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 78;}
	goto st731;
st731:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof731;
case 731:
#line 4192 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr187;
		case 13: goto st168;
		case 91: goto st169;
	}
	goto tr845;
st168:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof168;
case 168:
	if ( (*( sm->p)) == 10 )
		goto tr187;
	goto tr186;
st169:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof169;
case 169:
	if ( (*( sm->p)) == 47 )
		goto st170;
	goto tr188;
st170:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof170;
case 170:
	switch( (*( sm->p)) ) {
		case 83: goto st171;
		case 84: goto st179;
		case 115: goto st171;
		case 116: goto st179;
	}
	goto tr188;
st171:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof171;
case 171:
	switch( (*( sm->p)) ) {
		case 80: goto st172;
		case 112: goto st172;
	}
	goto tr188;
st172:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof172;
case 172:
	switch( (*( sm->p)) ) {
		case 79: goto st173;
		case 111: goto st173;
	}
	goto tr186;
st173:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof173;
case 173:
	switch( (*( sm->p)) ) {
		case 73: goto st174;
		case 105: goto st174;
	}
	goto tr186;
st174:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof174;
case 174:
	switch( (*( sm->p)) ) {
		case 76: goto st175;
		case 108: goto st175;
	}
	goto tr186;
st175:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof175;
case 175:
	switch( (*( sm->p)) ) {
		case 69: goto st176;
		case 101: goto st176;
	}
	goto tr186;
st176:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof176;
case 176:
	switch( (*( sm->p)) ) {
		case 82: goto st177;
		case 114: goto st177;
	}
	goto tr186;
st177:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof177;
case 177:
	switch( (*( sm->p)) ) {
		case 83: goto st178;
		case 93: goto tr199;
		case 115: goto st178;
	}
	goto tr186;
st178:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof178;
case 178:
	if ( (*( sm->p)) == 93 )
		goto tr199;
	goto tr186;
st179:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof179;
case 179:
	switch( (*( sm->p)) ) {
		case 68: goto st180;
		case 100: goto st180;
	}
	goto tr186;
st180:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof180;
case 180:
	if ( (*( sm->p)) == 93 )
		goto tr201;
	goto tr186;
tr842:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st181;
st181:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof181;
case 181:
#line 4317 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr203;
		case 32: goto tr203;
		case 42: goto st181;
	}
	goto tr202;
tr203:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st182;
st182:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof182;
case 182:
#line 4330 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr206;
		case 10: goto tr202;
		case 13: goto tr202;
		case 32: goto tr206;
	}
	goto tr205;
tr205:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st732;
st732:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof732;
case 732:
#line 4344 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 10: goto tr847;
		case 13: goto tr847;
	}
	goto st732;
tr206:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st733;
st733:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof733;
case 733:
#line 4356 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto tr206;
		case 10: goto tr847;
		case 13: goto tr847;
		case 32: goto tr206;
	}
	goto tr205;
st183:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof183;
case 183:
	if ( 49 <= (*( sm->p)) && (*( sm->p)) <= 54 )
		goto tr207;
	goto tr202;
tr207:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st184;
st184:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof184;
case 184:
#line 4377 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 46 )
		goto tr208;
	goto tr202;
tr208:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st734;
st734:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof734;
case 734:
#line 4387 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 9: goto st734;
		case 32: goto st734;
	}
	goto tr849;
st185:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof185;
case 185:
	if ( (*( sm->p)) == 47 )
		goto st186;
	goto tr202;
st186:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof186;
case 186:
	switch( (*( sm->p)) ) {
		case 81: goto st187;
		case 83: goto st192;
		case 84: goto st179;
		case 113: goto st187;
		case 115: goto st192;
		case 116: goto st179;
	}
	goto tr202;
st187:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof187;
case 187:
	switch( (*( sm->p)) ) {
		case 85: goto st188;
		case 117: goto st188;
	}
	goto tr186;
st188:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof188;
case 188:
	switch( (*( sm->p)) ) {
		case 79: goto st189;
		case 111: goto st189;
	}
	goto tr186;
st189:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof189;
case 189:
	switch( (*( sm->p)) ) {
		case 84: goto st190;
		case 116: goto st190;
	}
	goto tr186;
st190:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof190;
case 190:
	switch( (*( sm->p)) ) {
		case 69: goto st191;
		case 101: goto st191;
	}
	goto tr186;
st191:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof191;
case 191:
	_widec = (*( sm->p));
	if ( 93 <= (*( sm->p)) && (*( sm->p)) <= 93 ) {
		_widec = (short)(128 + ((*( sm->p)) - -128));
		if ( 
#line 98 "ext/dtext/dtext.cpp.rl"
 dstack_is_open(BLOCK_QUOTE)  ) _widec += 256;
	}
	if ( _widec == 605 )
		goto st735;
	goto tr186;
st735:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof735;
case 735:
	switch( (*( sm->p)) ) {
		case 9: goto st735;
		case 32: goto st735;
	}
	goto tr851;
st192:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof192;
case 192:
	switch( (*( sm->p)) ) {
		case 69: goto st193;
		case 80: goto st172;
		case 101: goto st193;
		case 112: goto st172;
	}
	goto tr202;
st193:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof193;
case 193:
	switch( (*( sm->p)) ) {
		case 67: goto st194;
		case 99: goto st194;
	}
	goto tr186;
st194:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof194;
case 194:
	switch( (*( sm->p)) ) {
		case 84: goto st195;
		case 116: goto st195;
	}
	goto tr186;
st195:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof195;
case 195:
	switch( (*( sm->p)) ) {
		case 73: goto st196;
		case 105: goto st196;
	}
	goto tr186;
st196:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof196;
case 196:
	switch( (*( sm->p)) ) {
		case 79: goto st197;
		case 111: goto st197;
	}
	goto tr186;
st197:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof197;
case 197:
	switch( (*( sm->p)) ) {
		case 78: goto st198;
		case 110: goto st198;
	}
	goto tr186;
st198:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof198;
case 198:
	_widec = (*( sm->p));
	if ( 93 <= (*( sm->p)) && (*( sm->p)) <= 93 ) {
		_widec = (short)(640 + ((*( sm->p)) - -128));
		if ( 
#line 99 "ext/dtext/dtext.cpp.rl"
 dstack_is_open(BLOCK_SECTION)  ) _widec += 256;
	}
	if ( _widec == 1117 )
		goto st736;
	goto tr186;
st736:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof736;
case 736:
	switch( (*( sm->p)) ) {
		case 9: goto st736;
		case 32: goto st736;
	}
	goto tr852;
st737:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof737;
case 737:
	if ( (*( sm->p)) == 10 )
		goto tr818;
	goto tr853;
tr820:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st738;
st738:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof738;
case 738:
#line 4562 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 34 )
		goto tr854;
	goto tr855;
tr855:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st199;
st199:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof199;
case 199:
#line 4572 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 34 )
		goto tr226;
	goto st199;
tr226:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st200;
st200:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof200;
case 200:
#line 4582 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 58 )
		goto st201;
	goto tr224;
st201:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof201;
case 201:
	switch( (*( sm->p)) ) {
		case 35: goto tr228;
		case 47: goto tr228;
		case 72: goto tr229;
		case 91: goto st210;
		case 104: goto tr229;
	}
	goto tr224;
tr228:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st202;
st202:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof202;
case 202:
#line 4604 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 32 )
		goto tr224;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr224;
	goto st739;
st739:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof739;
case 739:
	if ( (*( sm->p)) == 32 )
		goto tr856;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr856;
	goto st739;
tr229:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st203;
st203:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof203;
case 203:
#line 4625 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 84: goto st204;
		case 116: goto st204;
	}
	goto tr224;
st204:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof204;
case 204:
	switch( (*( sm->p)) ) {
		case 84: goto st205;
		case 116: goto st205;
	}
	goto tr224;
st205:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof205;
case 205:
	switch( (*( sm->p)) ) {
		case 80: goto st206;
		case 112: goto st206;
	}
	goto tr224;
st206:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof206;
case 206:
	switch( (*( sm->p)) ) {
		case 58: goto st207;
		case 83: goto st209;
		case 115: goto st209;
	}
	goto tr224;
st207:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof207;
case 207:
	if ( (*( sm->p)) == 47 )
		goto st208;
	goto tr224;
st208:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof208;
case 208:
	if ( (*( sm->p)) == 47 )
		goto st202;
	goto tr224;
st209:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof209;
case 209:
	if ( (*( sm->p)) == 58 )
		goto st207;
	goto tr224;
st210:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof210;
case 210:
	switch( (*( sm->p)) ) {
		case 35: goto tr239;
		case 47: goto tr239;
		case 72: goto tr240;
		case 104: goto tr240;
	}
	goto tr224;
tr239:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st211;
st211:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof211;
case 211:
#line 4697 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 32 )
		goto tr224;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr224;
	goto st212;
st212:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof212;
case 212:
	switch( (*( sm->p)) ) {
		case 32: goto tr224;
		case 93: goto tr242;
	}
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr224;
	goto st212;
tr240:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st213;
st213:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof213;
case 213:
#line 4720 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 84: goto st214;
		case 116: goto st214;
	}
	goto tr224;
st214:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof214;
case 214:
	switch( (*( sm->p)) ) {
		case 84: goto st215;
		case 116: goto st215;
	}
	goto tr224;
st215:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof215;
case 215:
	switch( (*( sm->p)) ) {
		case 80: goto st216;
		case 112: goto st216;
	}
	goto tr224;
st216:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof216;
case 216:
	switch( (*( sm->p)) ) {
		case 58: goto st217;
		case 83: goto st219;
		case 115: goto st219;
	}
	goto tr224;
st217:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof217;
case 217:
	if ( (*( sm->p)) == 47 )
		goto st218;
	goto tr224;
st218:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof218;
case 218:
	if ( (*( sm->p)) == 47 )
		goto st211;
	goto tr224;
st219:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof219;
case 219:
	if ( (*( sm->p)) == 58 )
		goto st217;
	goto tr224;
tr821:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st740;
st740:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof740;
case 740:
#line 4781 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto tr857;
		case 104: goto tr857;
	}
	goto tr854;
tr857:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st220;
st220:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof220;
case 220:
#line 4793 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 84: goto st221;
		case 116: goto st221;
	}
	goto tr224;
st221:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof221;
case 221:
	switch( (*( sm->p)) ) {
		case 84: goto st222;
		case 116: goto st222;
	}
	goto tr224;
st222:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof222;
case 222:
	switch( (*( sm->p)) ) {
		case 80: goto st223;
		case 112: goto st223;
	}
	goto tr224;
st223:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof223;
case 223:
	switch( (*( sm->p)) ) {
		case 58: goto st224;
		case 83: goto st228;
		case 115: goto st228;
	}
	goto tr224;
st224:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof224;
case 224:
	if ( (*( sm->p)) == 47 )
		goto st225;
	goto tr224;
st225:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof225;
case 225:
	if ( (*( sm->p)) == 47 )
		goto st226;
	goto tr224;
st226:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof226;
case 226:
	if ( (*( sm->p)) == 32 )
		goto tr224;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr224;
	goto st227;
st227:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof227;
case 227:
	switch( (*( sm->p)) ) {
		case 32: goto tr224;
		case 62: goto tr258;
	}
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr224;
	goto st227;
st228:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof228;
case 228:
	if ( (*( sm->p)) == 58 )
		goto st224;
	goto tr224;
tr822:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st741;
st741:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof741;
case 741:
#line 4874 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 76: goto st229;
		case 82: goto st235;
		case 108: goto st229;
		case 114: goto st235;
	}
	goto tr854;
st229:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof229;
case 229:
	switch( (*( sm->p)) ) {
		case 73: goto st230;
		case 105: goto st230;
	}
	goto tr224;
st230:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof230;
case 230:
	switch( (*( sm->p)) ) {
		case 65: goto st231;
		case 97: goto st231;
	}
	goto tr224;
st231:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof231;
case 231:
	switch( (*( sm->p)) ) {
		case 83: goto st232;
		case 115: goto st232;
	}
	goto tr224;
st232:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof232;
case 232:
	if ( (*( sm->p)) == 32 )
		goto st233;
	goto tr224;
st233:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof233;
case 233:
	if ( (*( sm->p)) == 35 )
		goto st234;
	goto tr224;
st234:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof234;
case 234:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr264;
	goto tr224;
tr264:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st742;
st742:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof742;
case 742:
#line 4936 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st742;
	goto tr860;
st235:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof235;
case 235:
	switch( (*( sm->p)) ) {
		case 84: goto st236;
		case 116: goto st236;
	}
	goto tr224;
st236:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof236;
case 236:
	switch( (*( sm->p)) ) {
		case 73: goto st237;
		case 105: goto st237;
	}
	goto tr224;
st237:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof237;
case 237:
	switch( (*( sm->p)) ) {
		case 83: goto st238;
		case 115: goto st238;
	}
	goto tr224;
st238:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof238;
case 238:
	switch( (*( sm->p)) ) {
		case 84: goto st239;
		case 116: goto st239;
	}
	goto tr224;
st239:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof239;
case 239:
	if ( (*( sm->p)) == 32 )
		goto st240;
	goto tr224;
st240:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof240;
case 240:
	if ( (*( sm->p)) == 35 )
		goto st241;
	goto tr224;
st241:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof241;
case 241:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr271;
	goto tr224;
tr271:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st743;
st743:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof743;
case 743:
#line 5003 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st743;
	goto tr862;
tr823:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st744;
st744:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof744;
case 744:
#line 5013 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 65: goto st242;
		case 76: goto st246;
		case 85: goto st251;
		case 97: goto st242;
		case 108: goto st246;
		case 117: goto st251;
	}
	goto tr854;
st242:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof242;
case 242:
	switch( (*( sm->p)) ) {
		case 78: goto st243;
		case 110: goto st243;
	}
	goto tr224;
st243:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof243;
case 243:
	if ( (*( sm->p)) == 32 )
		goto st244;
	goto tr224;
st244:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof244;
case 244:
	if ( (*( sm->p)) == 35 )
		goto st245;
	goto tr224;
st245:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof245;
case 245:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr275;
	goto tr224;
tr275:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st745;
st745:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof745;
case 745:
#line 5059 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st745;
	goto tr867;
st246:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof246;
case 246:
	switch( (*( sm->p)) ) {
		case 73: goto st247;
		case 105: goto st247;
	}
	goto tr224;
st247:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof247;
case 247:
	switch( (*( sm->p)) ) {
		case 80: goto st248;
		case 112: goto st248;
	}
	goto tr224;
st248:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof248;
case 248:
	if ( (*( sm->p)) == 32 )
		goto st249;
	goto tr224;
st249:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof249;
case 249:
	if ( (*( sm->p)) == 35 )
		goto st250;
	goto tr224;
st250:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof250;
case 250:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr280;
	goto tr224;
tr280:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st746;
st746:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof746;
case 746:
#line 5108 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st746;
	goto tr869;
st251:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof251;
case 251:
	switch( (*( sm->p)) ) {
		case 82: goto st252;
		case 114: goto st252;
	}
	goto tr224;
st252:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof252;
case 252:
	if ( (*( sm->p)) == 32 )
		goto st253;
	goto tr224;
st253:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof253;
case 253:
	if ( (*( sm->p)) == 35 )
		goto st254;
	goto tr224;
st254:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof254;
case 254:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr284;
	goto tr224;
tr284:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st747;
st747:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof747;
case 747:
#line 5148 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st747;
	goto tr871;
tr824:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st748;
st748:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof748;
case 748:
#line 5158 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st255;
		case 111: goto st255;
	}
	goto tr854;
st255:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof255;
case 255:
	switch( (*( sm->p)) ) {
		case 77: goto st256;
		case 109: goto st256;
	}
	goto tr224;
st256:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof256;
case 256:
	switch( (*( sm->p)) ) {
		case 77: goto st257;
		case 109: goto st257;
	}
	goto tr224;
st257:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof257;
case 257:
	switch( (*( sm->p)) ) {
		case 69: goto st258;
		case 101: goto st258;
	}
	goto tr224;
st258:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof258;
case 258:
	switch( (*( sm->p)) ) {
		case 78: goto st259;
		case 110: goto st259;
	}
	goto tr224;
st259:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof259;
case 259:
	switch( (*( sm->p)) ) {
		case 84: goto st260;
		case 116: goto st260;
	}
	goto tr224;
st260:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof260;
case 260:
	if ( (*( sm->p)) == 32 )
		goto st261;
	goto tr224;
st261:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof261;
case 261:
	if ( (*( sm->p)) == 35 )
		goto st262;
	goto tr224;
st262:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof262;
case 262:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr292;
	goto tr224;
tr292:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st749;
st749:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof749;
case 749:
#line 5236 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st749;
	goto tr874;
tr825:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st750;
st750:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof750;
case 750:
#line 5246 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 76: goto st263;
		case 79: goto st268;
		case 108: goto st263;
		case 111: goto st268;
	}
	goto tr854;
st263:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof263;
case 263:
	switch( (*( sm->p)) ) {
		case 65: goto st264;
		case 97: goto st264;
	}
	goto tr224;
st264:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof264;
case 264:
	switch( (*( sm->p)) ) {
		case 71: goto st265;
		case 103: goto st265;
	}
	goto tr224;
st265:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof265;
case 265:
	if ( (*( sm->p)) == 32 )
		goto st266;
	goto tr224;
st266:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof266;
case 266:
	if ( (*( sm->p)) == 35 )
		goto st267;
	goto tr224;
st267:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof267;
case 267:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr297;
	goto tr224;
tr297:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st751;
st751:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof751;
case 751:
#line 5299 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st751;
	goto tr878;
st268:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof268;
case 268:
	switch( (*( sm->p)) ) {
		case 82: goto st269;
		case 114: goto st269;
	}
	goto tr224;
st269:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof269;
case 269:
	switch( (*( sm->p)) ) {
		case 85: goto st270;
		case 117: goto st270;
	}
	goto tr224;
st270:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof270;
case 270:
	switch( (*( sm->p)) ) {
		case 77: goto st271;
		case 109: goto st271;
	}
	goto tr224;
st271:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof271;
case 271:
	if ( (*( sm->p)) == 32 )
		goto st272;
	goto tr224;
st272:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof272;
case 272:
	if ( (*( sm->p)) == 35 )
		goto st273;
	goto tr224;
st273:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof273;
case 273:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr303;
	goto tr224;
tr303:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st752;
st752:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof752;
case 752:
#line 5357 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st752;
	goto tr880;
tr826:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st753;
st753:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof753;
case 753:
#line 5367 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 84: goto st274;
		case 116: goto st274;
	}
	goto tr854;
st274:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof274;
case 274:
	switch( (*( sm->p)) ) {
		case 84: goto st275;
		case 116: goto st275;
	}
	goto tr224;
st275:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof275;
case 275:
	switch( (*( sm->p)) ) {
		case 80: goto st276;
		case 112: goto st276;
	}
	goto tr224;
st276:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof276;
case 276:
	switch( (*( sm->p)) ) {
		case 58: goto st277;
		case 83: goto st280;
		case 115: goto st280;
	}
	goto tr224;
st277:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof277;
case 277:
	if ( (*( sm->p)) == 47 )
		goto st278;
	goto tr224;
st278:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof278;
case 278:
	if ( (*( sm->p)) == 47 )
		goto st279;
	goto tr224;
st279:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof279;
case 279:
	if ( (*( sm->p)) == 32 )
		goto tr224;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr224;
	goto st754;
st754:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof754;
case 754:
	if ( (*( sm->p)) == 32 )
		goto tr883;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto tr883;
	goto st754;
st280:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof280;
case 280:
	if ( (*( sm->p)) == 58 )
		goto st277;
	goto tr224;
tr827:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st755;
st755:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof755;
case 755:
#line 5446 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 77: goto st281;
		case 109: goto st281;
	}
	goto tr854;
st281:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof281;
case 281:
	switch( (*( sm->p)) ) {
		case 80: goto st282;
		case 112: goto st282;
	}
	goto tr224;
st282:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof282;
case 282:
	switch( (*( sm->p)) ) {
		case 76: goto st283;
		case 108: goto st283;
	}
	goto tr224;
st283:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof283;
case 283:
	switch( (*( sm->p)) ) {
		case 73: goto st284;
		case 105: goto st284;
	}
	goto tr224;
st284:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof284;
case 284:
	switch( (*( sm->p)) ) {
		case 67: goto st285;
		case 99: goto st285;
	}
	goto tr224;
st285:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof285;
case 285:
	switch( (*( sm->p)) ) {
		case 65: goto st286;
		case 97: goto st286;
	}
	goto tr224;
st286:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof286;
case 286:
	switch( (*( sm->p)) ) {
		case 84: goto st287;
		case 116: goto st287;
	}
	goto tr224;
st287:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof287;
case 287:
	switch( (*( sm->p)) ) {
		case 73: goto st288;
		case 105: goto st288;
	}
	goto tr224;
st288:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof288;
case 288:
	switch( (*( sm->p)) ) {
		case 79: goto st289;
		case 111: goto st289;
	}
	goto tr224;
st289:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof289;
case 289:
	switch( (*( sm->p)) ) {
		case 78: goto st290;
		case 110: goto st290;
	}
	goto tr224;
st290:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof290;
case 290:
	if ( (*( sm->p)) == 32 )
		goto st291;
	goto tr224;
st291:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof291;
case 291:
	if ( (*( sm->p)) == 35 )
		goto st292;
	goto tr224;
st292:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof292;
case 292:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr322;
	goto tr224;
tr322:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st756;
st756:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof756;
case 756:
#line 5560 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st756;
	goto tr885;
tr828:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st757;
st757:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof757;
case 757:
#line 5570 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st293;
		case 111: goto st293;
	}
	goto tr854;
st293:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof293;
case 293:
	switch( (*( sm->p)) ) {
		case 68: goto st294;
		case 100: goto st294;
	}
	goto tr224;
st294:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof294;
case 294:
	if ( (*( sm->p)) == 32 )
		goto st295;
	goto tr224;
st295:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof295;
case 295:
	switch( (*( sm->p)) ) {
		case 65: goto st296;
		case 97: goto st296;
	}
	goto tr224;
st296:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof296;
case 296:
	switch( (*( sm->p)) ) {
		case 67: goto st297;
		case 99: goto st297;
	}
	goto tr224;
st297:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof297;
case 297:
	switch( (*( sm->p)) ) {
		case 84: goto st298;
		case 116: goto st298;
	}
	goto tr224;
st298:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof298;
case 298:
	switch( (*( sm->p)) ) {
		case 73: goto st299;
		case 105: goto st299;
	}
	goto tr224;
st299:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof299;
case 299:
	switch( (*( sm->p)) ) {
		case 79: goto st300;
		case 111: goto st300;
	}
	goto tr224;
st300:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof300;
case 300:
	switch( (*( sm->p)) ) {
		case 78: goto st301;
		case 110: goto st301;
	}
	goto tr224;
st301:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof301;
case 301:
	if ( (*( sm->p)) == 32 )
		goto st302;
	goto tr224;
st302:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof302;
case 302:
	if ( (*( sm->p)) == 35 )
		goto st303;
	goto tr224;
st303:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof303;
case 303:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr333;
	goto tr224;
tr333:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st758;
st758:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof758;
case 758:
#line 5673 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st758;
	goto tr888;
tr829:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st759;
st759:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof759;
case 759:
#line 5683 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st304;
		case 111: goto st304;
	}
	goto tr854;
st304:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof304;
case 304:
	switch( (*( sm->p)) ) {
		case 84: goto st305;
		case 116: goto st305;
	}
	goto tr224;
st305:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof305;
case 305:
	switch( (*( sm->p)) ) {
		case 69: goto st306;
		case 101: goto st306;
	}
	goto tr224;
st306:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof306;
case 306:
	if ( (*( sm->p)) == 32 )
		goto st307;
	goto tr224;
st307:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof307;
case 307:
	if ( (*( sm->p)) == 35 )
		goto st308;
	goto tr224;
st308:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof308;
case 308:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr338;
	goto tr224;
tr338:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st760;
st760:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof760;
case 760:
#line 5734 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st760;
	goto tr891;
tr830:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st761;
st761:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof761;
case 761:
#line 5744 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st309;
		case 111: goto st309;
	}
	goto tr854;
st309:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof309;
case 309:
	switch( (*( sm->p)) ) {
		case 79: goto st310;
		case 83: goto st314;
		case 111: goto st310;
		case 115: goto st314;
	}
	goto tr224;
st310:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof310;
case 310:
	switch( (*( sm->p)) ) {
		case 76: goto st311;
		case 108: goto st311;
	}
	goto tr224;
st311:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof311;
case 311:
	if ( (*( sm->p)) == 32 )
		goto st312;
	goto tr224;
st312:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof312;
case 312:
	if ( (*( sm->p)) == 35 )
		goto st313;
	goto tr224;
st313:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof313;
case 313:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr344;
	goto tr224;
tr344:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st762;
st762:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof762;
case 762:
#line 5797 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st762;
	goto tr894;
st314:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof314;
case 314:
	switch( (*( sm->p)) ) {
		case 84: goto st315;
		case 116: goto st315;
	}
	goto tr224;
st315:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof315;
case 315:
	if ( (*( sm->p)) == 32 )
		goto st316;
	goto tr224;
st316:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof316;
case 316:
	switch( (*( sm->p)) ) {
		case 35: goto st317;
		case 67: goto st318;
		case 99: goto st318;
	}
	goto tr224;
st317:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof317;
case 317:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr349;
	goto tr224;
tr349:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st763;
st763:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof763;
case 763:
#line 5840 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st763;
	goto tr896;
st318:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof318;
case 318:
	switch( (*( sm->p)) ) {
		case 72: goto st319;
		case 104: goto st319;
	}
	goto tr224;
st319:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof319;
case 319:
	switch( (*( sm->p)) ) {
		case 65: goto st320;
		case 97: goto st320;
	}
	goto tr224;
st320:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof320;
case 320:
	switch( (*( sm->p)) ) {
		case 78: goto st321;
		case 110: goto st321;
	}
	goto tr224;
st321:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof321;
case 321:
	switch( (*( sm->p)) ) {
		case 71: goto st322;
		case 103: goto st322;
	}
	goto tr224;
st322:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof322;
case 322:
	switch( (*( sm->p)) ) {
		case 69: goto st323;
		case 101: goto st323;
	}
	goto tr224;
st323:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof323;
case 323:
	switch( (*( sm->p)) ) {
		case 83: goto st324;
		case 115: goto st324;
	}
	goto tr224;
st324:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof324;
case 324:
	if ( (*( sm->p)) == 32 )
		goto st325;
	goto tr224;
st325:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof325;
case 325:
	if ( (*( sm->p)) == 35 )
		goto st326;
	goto tr224;
st326:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof326;
case 326:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr358;
	goto tr224;
tr358:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st764;
st764:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof764;
case 764:
#line 5925 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st764;
	goto tr898;
tr831:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st765;
st765:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof765;
case 765:
#line 5935 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st327;
		case 101: goto st327;
	}
	goto tr854;
st327:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof327;
case 327:
	switch( (*( sm->p)) ) {
		case 67: goto st328;
		case 99: goto st328;
	}
	goto tr224;
st328:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof328;
case 328:
	switch( (*( sm->p)) ) {
		case 79: goto st329;
		case 111: goto st329;
	}
	goto tr224;
st329:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof329;
case 329:
	switch( (*( sm->p)) ) {
		case 82: goto st330;
		case 114: goto st330;
	}
	goto tr224;
st330:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof330;
case 330:
	switch( (*( sm->p)) ) {
		case 68: goto st331;
		case 100: goto st331;
	}
	goto tr224;
st331:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof331;
case 331:
	if ( (*( sm->p)) == 32 )
		goto st332;
	goto tr224;
st332:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof332;
case 332:
	if ( (*( sm->p)) == 35 )
		goto st333;
	goto tr224;
st333:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof333;
case 333:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr365;
	goto tr224;
tr365:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st766;
st766:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof766;
case 766:
#line 6004 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st766;
	goto tr901;
tr832:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st767;
st767:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof767;
case 767:
#line 6014 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st334;
		case 101: goto st334;
	}
	goto tr854;
st334:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof334;
case 334:
	switch( (*( sm->p)) ) {
		case 84: goto st335;
		case 116: goto st335;
	}
	goto tr224;
st335:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof335;
case 335:
	if ( (*( sm->p)) == 32 )
		goto st336;
	goto tr224;
st336:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof336;
case 336:
	if ( (*( sm->p)) == 35 )
		goto st337;
	goto tr224;
st337:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof337;
case 337:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr369;
	goto tr224;
tr369:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st768;
st768:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof768;
case 768:
#line 6056 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st768;
	goto tr904;
tr833:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st769;
st769:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof769;
case 769:
#line 6066 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 65: goto st338;
		case 72: goto st356;
		case 73: goto st362;
		case 79: goto st369;
		case 97: goto st338;
		case 104: goto st356;
		case 105: goto st362;
		case 111: goto st369;
	}
	goto tr854;
st338:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof338;
case 338:
	switch( (*( sm->p)) ) {
		case 75: goto st339;
		case 107: goto st339;
	}
	goto tr224;
st339:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof339;
case 339:
	switch( (*( sm->p)) ) {
		case 69: goto st340;
		case 101: goto st340;
	}
	goto tr224;
st340:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof340;
case 340:
	switch( (*( sm->p)) ) {
		case 32: goto st341;
		case 68: goto st342;
		case 100: goto st342;
	}
	goto tr224;
st341:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof341;
case 341:
	switch( (*( sm->p)) ) {
		case 68: goto st342;
		case 100: goto st342;
	}
	goto tr224;
st342:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof342;
case 342:
	switch( (*( sm->p)) ) {
		case 79: goto st343;
		case 111: goto st343;
	}
	goto tr224;
st343:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof343;
case 343:
	switch( (*( sm->p)) ) {
		case 87: goto st344;
		case 119: goto st344;
	}
	goto tr224;
st344:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof344;
case 344:
	switch( (*( sm->p)) ) {
		case 78: goto st345;
		case 110: goto st345;
	}
	goto tr224;
st345:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof345;
case 345:
	if ( (*( sm->p)) == 32 )
		goto st346;
	goto tr224;
st346:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof346;
case 346:
	switch( (*( sm->p)) ) {
		case 35: goto st347;
		case 82: goto st348;
		case 114: goto st348;
	}
	goto tr224;
st347:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof347;
case 347:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr380;
	goto tr224;
tr380:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st770;
st770:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof770;
case 770:
#line 6172 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st770;
	goto tr910;
st348:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof348;
case 348:
	switch( (*( sm->p)) ) {
		case 69: goto st349;
		case 101: goto st349;
	}
	goto tr224;
st349:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof349;
case 349:
	switch( (*( sm->p)) ) {
		case 81: goto st350;
		case 113: goto st350;
	}
	goto tr224;
st350:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof350;
case 350:
	switch( (*( sm->p)) ) {
		case 85: goto st351;
		case 117: goto st351;
	}
	goto tr224;
st351:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof351;
case 351:
	switch( (*( sm->p)) ) {
		case 69: goto st352;
		case 101: goto st352;
	}
	goto tr224;
st352:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof352;
case 352:
	switch( (*( sm->p)) ) {
		case 83: goto st353;
		case 115: goto st353;
	}
	goto tr224;
st353:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof353;
case 353:
	switch( (*( sm->p)) ) {
		case 84: goto st354;
		case 116: goto st354;
	}
	goto tr224;
st354:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof354;
case 354:
	if ( (*( sm->p)) == 32 )
		goto st355;
	goto tr224;
st355:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof355;
case 355:
	if ( (*( sm->p)) == 35 )
		goto st347;
	goto tr224;
st356:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof356;
case 356:
	switch( (*( sm->p)) ) {
		case 85: goto st357;
		case 117: goto st357;
	}
	goto tr224;
st357:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof357;
case 357:
	switch( (*( sm->p)) ) {
		case 77: goto st358;
		case 109: goto st358;
	}
	goto tr224;
st358:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof358;
case 358:
	switch( (*( sm->p)) ) {
		case 66: goto st359;
		case 98: goto st359;
	}
	goto tr224;
st359:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof359;
case 359:
	if ( (*( sm->p)) == 32 )
		goto st360;
	goto tr224;
st360:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof360;
case 360:
	if ( (*( sm->p)) == 35 )
		goto st361;
	goto tr224;
st361:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof361;
case 361:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr393;
	goto tr224;
tr393:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st771;
st771:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof771;
case 771:
#line 6298 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st771;
	goto tr912;
st362:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof362;
case 362:
	switch( (*( sm->p)) ) {
		case 67: goto st363;
		case 99: goto st363;
	}
	goto tr224;
st363:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof363;
case 363:
	switch( (*( sm->p)) ) {
		case 75: goto st364;
		case 107: goto st364;
	}
	goto tr224;
st364:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof364;
case 364:
	switch( (*( sm->p)) ) {
		case 69: goto st365;
		case 101: goto st365;
	}
	goto tr224;
st365:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof365;
case 365:
	switch( (*( sm->p)) ) {
		case 84: goto st366;
		case 116: goto st366;
	}
	goto tr224;
st366:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof366;
case 366:
	if ( (*( sm->p)) == 32 )
		goto st367;
	goto tr224;
st367:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof367;
case 367:
	if ( (*( sm->p)) == 35 )
		goto st368;
	goto tr224;
st368:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof368;
case 368:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr400;
	goto tr224;
tr400:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st772;
st772:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof772;
case 772:
#line 6365 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st772;
	goto tr914;
st369:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof369;
case 369:
	switch( (*( sm->p)) ) {
		case 80: goto st370;
		case 112: goto st370;
	}
	goto tr224;
st370:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof370;
case 370:
	switch( (*( sm->p)) ) {
		case 73: goto st371;
		case 105: goto st371;
	}
	goto tr224;
st371:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof371;
case 371:
	switch( (*( sm->p)) ) {
		case 67: goto st372;
		case 99: goto st372;
	}
	goto tr224;
st372:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof372;
case 372:
	if ( (*( sm->p)) == 32 )
		goto st373;
	goto tr224;
st373:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof373;
case 373:
	if ( (*( sm->p)) == 35 )
		goto st374;
	goto tr224;
st374:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof374;
case 374:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr406;
	goto tr224;
tr406:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st773;
st773:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof773;
case 773:
#line 6423 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st773;
	goto tr916;
tr834:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st774;
st774:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof774;
case 774:
#line 6433 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 83: goto st375;
		case 115: goto st375;
	}
	goto tr854;
st375:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof375;
case 375:
	switch( (*( sm->p)) ) {
		case 69: goto st376;
		case 101: goto st376;
	}
	goto tr224;
st376:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof376;
case 376:
	switch( (*( sm->p)) ) {
		case 82: goto st377;
		case 114: goto st377;
	}
	goto tr224;
st377:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof377;
case 377:
	if ( (*( sm->p)) == 32 )
		goto st378;
	goto tr224;
st378:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof378;
case 378:
	if ( (*( sm->p)) == 35 )
		goto st379;
	goto tr224;
st379:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof379;
case 379:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr411;
	goto tr224;
tr411:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st775;
st775:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof775;
case 775:
#line 6484 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st775;
	goto tr919;
tr835:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st776;
st776:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof776;
case 776:
#line 6494 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 73: goto st380;
		case 105: goto st380;
	}
	goto tr854;
st380:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof380;
case 380:
	switch( (*( sm->p)) ) {
		case 75: goto st381;
		case 107: goto st381;
	}
	goto tr224;
st381:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof381;
case 381:
	switch( (*( sm->p)) ) {
		case 73: goto st382;
		case 105: goto st382;
	}
	goto tr224;
st382:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof382;
case 382:
	if ( (*( sm->p)) == 32 )
		goto st383;
	goto tr224;
st383:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof383;
case 383:
	if ( (*( sm->p)) == 35 )
		goto st384;
	goto tr224;
st384:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof384;
case 384:
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto tr416;
	goto tr224;
tr416:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st777;
st777:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof777;
case 777:
#line 6545 "ext/dtext/dtext.cpp"
	if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
		goto st777;
	goto tr922;
tr836:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
#line 516 "ext/dtext/dtext.cpp.rl"
	{( sm->act) = 81;}
	goto st778;
st778:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof778;
case 778:
#line 6556 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 35: goto st385;
		case 47: goto st387;
		case 66: goto st408;
		case 67: goto st409;
		case 73: goto st527;
		case 81: goto st528;
		case 83: goto st644;
		case 84: goto st662;
		case 85: goto st667;
		case 91: goto st668;
		case 98: goto st408;
		case 99: goto st409;
		case 105: goto st527;
		case 113: goto st528;
		case 115: goto st644;
		case 116: goto st662;
		case 117: goto st667;
	}
	goto tr854;
st385:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof385;
case 385:
	switch( (*( sm->p)) ) {
		case 45: goto tr417;
		case 95: goto tr417;
	}
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto tr417;
	} else if ( (*( sm->p)) > 90 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
			goto tr417;
	} else
		goto tr417;
	goto tr224;
tr417:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st386;
st386:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof386;
case 386:
#line 6600 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 45: goto st386;
		case 93: goto tr419;
		case 95: goto st386;
	}
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st386;
	} else if ( (*( sm->p)) > 90 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
			goto st386;
	} else
		goto st386;
	goto tr224;
st387:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof387;
case 387:
	switch( (*( sm->p)) ) {
		case 66: goto st388;
		case 67: goto st389;
		case 73: goto st396;
		case 81: goto st187;
		case 83: goto st397;
		case 84: goto st401;
		case 85: goto st407;
		case 98: goto st388;
		case 99: goto st389;
		case 105: goto st396;
		case 113: goto st187;
		case 115: goto st397;
		case 116: goto st401;
		case 117: goto st407;
	}
	goto tr224;
st388:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof388;
case 388:
	if ( (*( sm->p)) == 93 )
		goto tr426;
	goto tr224;
st389:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof389;
case 389:
	switch( (*( sm->p)) ) {
		case 79: goto st390;
		case 111: goto st390;
	}
	goto tr224;
st390:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof390;
case 390:
	switch( (*( sm->p)) ) {
		case 68: goto st391;
		case 76: goto st393;
		case 100: goto st391;
		case 108: goto st393;
	}
	goto tr224;
st391:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof391;
case 391:
	switch( (*( sm->p)) ) {
		case 69: goto st392;
		case 101: goto st392;
	}
	goto tr224;
st392:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof392;
case 392:
	if ( (*( sm->p)) == 93 )
		goto st779;
	goto tr224;
st779:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof779;
case 779:
	if ( (*( sm->p)) == 32 )
		goto st779;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st779;
	goto tr934;
st393:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof393;
case 393:
	switch( (*( sm->p)) ) {
		case 79: goto st394;
		case 111: goto st394;
	}
	goto tr224;
st394:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof394;
case 394:
	switch( (*( sm->p)) ) {
		case 82: goto st395;
		case 114: goto st395;
	}
	goto tr224;
st395:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof395;
case 395:
	if ( (*( sm->p)) == 93 )
		goto tr434;
	goto tr224;
st396:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof396;
case 396:
	if ( (*( sm->p)) == 93 )
		goto tr435;
	goto tr224;
st397:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof397;
case 397:
	switch( (*( sm->p)) ) {
		case 69: goto st193;
		case 80: goto st172;
		case 85: goto st398;
		case 93: goto tr437;
		case 101: goto st193;
		case 112: goto st172;
		case 117: goto st398;
	}
	goto tr224;
st398:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof398;
case 398:
	switch( (*( sm->p)) ) {
		case 66: goto st399;
		case 80: goto st400;
		case 98: goto st399;
		case 112: goto st400;
	}
	goto tr224;
st399:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof399;
case 399:
	if ( (*( sm->p)) == 93 )
		goto tr440;
	goto tr224;
st400:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof400;
case 400:
	if ( (*( sm->p)) == 93 )
		goto tr441;
	goto tr224;
st401:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof401;
case 401:
	switch( (*( sm->p)) ) {
		case 65: goto st402;
		case 68: goto st180;
		case 72: goto st406;
		case 97: goto st402;
		case 100: goto st180;
		case 104: goto st406;
	}
	goto tr224;
st402:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof402;
case 402:
	switch( (*( sm->p)) ) {
		case 66: goto st403;
		case 98: goto st403;
	}
	goto tr224;
st403:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof403;
case 403:
	switch( (*( sm->p)) ) {
		case 76: goto st404;
		case 108: goto st404;
	}
	goto tr224;
st404:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof404;
case 404:
	switch( (*( sm->p)) ) {
		case 69: goto st405;
		case 101: goto st405;
	}
	goto tr224;
st405:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof405;
case 405:
	if ( (*( sm->p)) == 93 )
		goto st780;
	goto tr224;
st780:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof780;
case 780:
	if ( (*( sm->p)) == 32 )
		goto st780;
	if ( 9 <= (*( sm->p)) && (*( sm->p)) <= 13 )
		goto st780;
	goto tr935;
st406:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof406;
case 406:
	if ( (*( sm->p)) == 93 )
		goto tr448;
	goto tr224;
st407:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof407;
case 407:
	if ( (*( sm->p)) == 93 )
		goto tr449;
	goto tr224;
st408:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof408;
case 408:
	if ( (*( sm->p)) == 93 )
		goto tr450;
	goto tr224;
st409:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof409;
case 409:
	switch( (*( sm->p)) ) {
		case 79: goto st410;
		case 111: goto st410;
	}
	goto tr224;
st410:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof410;
case 410:
	switch( (*( sm->p)) ) {
		case 68: goto st411;
		case 76: goto st413;
		case 100: goto st411;
		case 108: goto st413;
	}
	goto tr224;
st411:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof411;
case 411:
	switch( (*( sm->p)) ) {
		case 69: goto st412;
		case 101: goto st412;
	}
	goto tr224;
st412:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof412;
case 412:
	if ( (*( sm->p)) == 93 )
		goto tr455;
	goto tr224;
st413:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof413;
case 413:
	switch( (*( sm->p)) ) {
		case 79: goto st414;
		case 111: goto st414;
	}
	goto tr224;
st414:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof414;
case 414:
	switch( (*( sm->p)) ) {
		case 82: goto st415;
		case 114: goto st415;
	}
	goto tr224;
st415:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof415;
case 415:
	if ( (*( sm->p)) == 61 )
		goto st416;
	goto tr224;
st416:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof416;
case 416:
	switch( (*( sm->p)) ) {
		case 35: goto tr459;
		case 65: goto tr460;
		case 67: goto tr461;
		case 71: goto tr462;
		case 73: goto tr463;
		case 76: goto tr464;
		case 77: goto tr465;
		case 83: goto tr466;
		case 97: goto tr467;
		case 99: goto tr469;
		case 103: goto tr470;
		case 105: goto tr471;
		case 108: goto tr472;
		case 109: goto tr473;
		case 115: goto tr474;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto tr468;
	goto tr224;
tr459:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st417;
st417:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof417;
case 417:
#line 6927 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st418;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st418;
	} else
		goto st418;
	goto tr224;
st418:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof418;
case 418:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st419;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st419;
	} else
		goto st419;
	goto tr224;
st419:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof419;
case 419:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st420;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st420;
	} else
		goto st420;
	goto tr224;
st420:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof420;
case 420:
	if ( (*( sm->p)) == 93 )
		goto tr479;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st421;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st421;
	} else
		goto st421;
	goto tr224;
st421:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof421;
case 421:
	if ( (*( sm->p)) == 93 )
		goto tr479;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st422;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st422;
	} else
		goto st422;
	goto tr224;
st422:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof422;
case 422:
	if ( (*( sm->p)) == 93 )
		goto tr479;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st423;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st423;
	} else
		goto st423;
	goto tr224;
st423:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof423;
case 423:
	if ( (*( sm->p)) == 93 )
		goto tr479;
	goto tr224;
tr460:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st424;
st424:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof424;
case 424:
#line 7021 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st425;
		case 114: goto st425;
	}
	goto tr224;
st425:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof425;
case 425:
	switch( (*( sm->p)) ) {
		case 84: goto st426;
		case 116: goto st426;
	}
	goto tr224;
st426:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof426;
case 426:
	switch( (*( sm->p)) ) {
		case 73: goto st427;
		case 93: goto tr485;
		case 105: goto st427;
	}
	goto tr224;
st427:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof427;
case 427:
	switch( (*( sm->p)) ) {
		case 83: goto st428;
		case 115: goto st428;
	}
	goto tr224;
st428:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof428;
case 428:
	switch( (*( sm->p)) ) {
		case 84: goto st429;
		case 116: goto st429;
	}
	goto tr224;
st429:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof429;
case 429:
	if ( (*( sm->p)) == 93 )
		goto tr485;
	goto tr224;
tr461:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st430;
st430:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof430;
case 430:
#line 7077 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st431;
		case 79: goto st438;
		case 104: goto st431;
		case 111: goto st438;
	}
	goto tr224;
st431:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof431;
case 431:
	switch( (*( sm->p)) ) {
		case 65: goto st432;
		case 97: goto st432;
	}
	goto tr224;
st432:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof432;
case 432:
	switch( (*( sm->p)) ) {
		case 82: goto st433;
		case 114: goto st433;
	}
	goto tr224;
st433:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof433;
case 433:
	switch( (*( sm->p)) ) {
		case 65: goto st434;
		case 93: goto tr485;
		case 97: goto st434;
	}
	goto tr224;
st434:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof434;
case 434:
	switch( (*( sm->p)) ) {
		case 67: goto st435;
		case 99: goto st435;
	}
	goto tr224;
st435:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof435;
case 435:
	switch( (*( sm->p)) ) {
		case 84: goto st436;
		case 116: goto st436;
	}
	goto tr224;
st436:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof436;
case 436:
	switch( (*( sm->p)) ) {
		case 69: goto st437;
		case 101: goto st437;
	}
	goto tr224;
st437:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof437;
case 437:
	switch( (*( sm->p)) ) {
		case 82: goto st429;
		case 114: goto st429;
	}
	goto tr224;
st438:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof438;
case 438:
	switch( (*( sm->p)) ) {
		case 78: goto st439;
		case 80: goto st446;
		case 110: goto st439;
		case 112: goto st446;
	}
	goto tr224;
st439:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof439;
case 439:
	switch( (*( sm->p)) ) {
		case 84: goto st440;
		case 116: goto st440;
	}
	goto tr224;
st440:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof440;
case 440:
	switch( (*( sm->p)) ) {
		case 82: goto st441;
		case 93: goto tr485;
		case 114: goto st441;
	}
	goto tr224;
st441:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof441;
case 441:
	switch( (*( sm->p)) ) {
		case 73: goto st442;
		case 105: goto st442;
	}
	goto tr224;
st442:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof442;
case 442:
	switch( (*( sm->p)) ) {
		case 66: goto st443;
		case 98: goto st443;
	}
	goto tr224;
st443:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof443;
case 443:
	switch( (*( sm->p)) ) {
		case 85: goto st444;
		case 117: goto st444;
	}
	goto tr224;
st444:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof444;
case 444:
	switch( (*( sm->p)) ) {
		case 84: goto st445;
		case 116: goto st445;
	}
	goto tr224;
st445:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof445;
case 445:
	switch( (*( sm->p)) ) {
		case 79: goto st437;
		case 111: goto st437;
	}
	goto tr224;
st446:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof446;
case 446:
	switch( (*( sm->p)) ) {
		case 89: goto st447;
		case 121: goto st447;
	}
	goto tr224;
st447:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof447;
case 447:
	switch( (*( sm->p)) ) {
		case 82: goto st448;
		case 93: goto tr485;
		case 114: goto st448;
	}
	goto tr224;
st448:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof448;
case 448:
	switch( (*( sm->p)) ) {
		case 73: goto st449;
		case 105: goto st449;
	}
	goto tr224;
st449:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof449;
case 449:
	switch( (*( sm->p)) ) {
		case 71: goto st450;
		case 103: goto st450;
	}
	goto tr224;
st450:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof450;
case 450:
	switch( (*( sm->p)) ) {
		case 72: goto st428;
		case 104: goto st428;
	}
	goto tr224;
tr462:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st451;
st451:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof451;
case 451:
#line 7276 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st452;
		case 101: goto st452;
	}
	goto tr224;
st452:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof452;
case 452:
	switch( (*( sm->p)) ) {
		case 78: goto st453;
		case 110: goto st453;
	}
	goto tr224;
st453:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof453;
case 453:
	switch( (*( sm->p)) ) {
		case 69: goto st454;
		case 93: goto tr485;
		case 101: goto st454;
	}
	goto tr224;
st454:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof454;
case 454:
	switch( (*( sm->p)) ) {
		case 82: goto st455;
		case 114: goto st455;
	}
	goto tr224;
st455:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof455;
case 455:
	switch( (*( sm->p)) ) {
		case 65: goto st456;
		case 97: goto st456;
	}
	goto tr224;
st456:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof456;
case 456:
	switch( (*( sm->p)) ) {
		case 76: goto st429;
		case 108: goto st429;
	}
	goto tr224;
tr463:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st457;
st457:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof457;
case 457:
#line 7334 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st458;
		case 110: goto st458;
	}
	goto tr224;
st458:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof458;
case 458:
	switch( (*( sm->p)) ) {
		case 86: goto st459;
		case 118: goto st459;
	}
	goto tr224;
st459:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof459;
case 459:
	switch( (*( sm->p)) ) {
		case 65: goto st460;
		case 93: goto tr485;
		case 97: goto st460;
	}
	goto tr224;
st460:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof460;
case 460:
	switch( (*( sm->p)) ) {
		case 76: goto st461;
		case 108: goto st461;
	}
	goto tr224;
st461:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof461;
case 461:
	switch( (*( sm->p)) ) {
		case 73: goto st462;
		case 105: goto st462;
	}
	goto tr224;
st462:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof462;
case 462:
	switch( (*( sm->p)) ) {
		case 68: goto st429;
		case 100: goto st429;
	}
	goto tr224;
tr464:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st463;
st463:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof463;
case 463:
#line 7392 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st464;
		case 111: goto st464;
	}
	goto tr224;
st464:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof464;
case 464:
	switch( (*( sm->p)) ) {
		case 82: goto st465;
		case 114: goto st465;
	}
	goto tr224;
st465:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof465;
case 465:
	switch( (*( sm->p)) ) {
		case 69: goto st429;
		case 93: goto tr485;
		case 101: goto st429;
	}
	goto tr224;
tr465:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st466;
st466:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof466;
case 466:
#line 7423 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st467;
		case 101: goto st467;
	}
	goto tr224;
st467:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof467;
case 467:
	switch( (*( sm->p)) ) {
		case 84: goto st468;
		case 116: goto st468;
	}
	goto tr224;
st468:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof468;
case 468:
	switch( (*( sm->p)) ) {
		case 65: goto st429;
		case 97: goto st429;
	}
	goto tr224;
tr466:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st469;
st469:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof469;
case 469:
#line 7453 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st470;
		case 112: goto st470;
	}
	goto tr224;
st470:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof470;
case 470:
	switch( (*( sm->p)) ) {
		case 69: goto st471;
		case 101: goto st471;
	}
	goto tr224;
st471:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof471;
case 471:
	switch( (*( sm->p)) ) {
		case 67: goto st472;
		case 99: goto st472;
	}
	goto tr224;
st472:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof472;
case 472:
	switch( (*( sm->p)) ) {
		case 73: goto st473;
		case 93: goto tr485;
		case 105: goto st473;
	}
	goto tr224;
st473:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof473;
case 473:
	switch( (*( sm->p)) ) {
		case 69: goto st474;
		case 101: goto st474;
	}
	goto tr224;
st474:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof474;
case 474:
	switch( (*( sm->p)) ) {
		case 83: goto st429;
		case 115: goto st429;
	}
	goto tr224;
tr467:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st475;
st475:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof475;
case 475:
#line 7511 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st425;
		case 93: goto tr479;
		case 114: goto st477;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
tr468:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st476;
st476:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof476;
case 476:
#line 7526 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr479;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st477:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof477;
case 477:
	switch( (*( sm->p)) ) {
		case 84: goto st426;
		case 93: goto tr479;
		case 116: goto st478;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st478:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof478;
case 478:
	switch( (*( sm->p)) ) {
		case 73: goto st427;
		case 93: goto tr485;
		case 105: goto st479;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st479:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof479;
case 479:
	switch( (*( sm->p)) ) {
		case 83: goto st428;
		case 93: goto tr479;
		case 115: goto st480;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st480:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof480;
case 480:
	switch( (*( sm->p)) ) {
		case 84: goto st429;
		case 93: goto tr479;
		case 116: goto st481;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st481:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof481;
case 481:
	if ( (*( sm->p)) == 93 )
		goto tr485;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
tr469:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st482;
st482:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof482;
case 482:
#line 7595 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st431;
		case 79: goto st438;
		case 93: goto tr479;
		case 104: goto st483;
		case 111: goto st490;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st483:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof483;
case 483:
	switch( (*( sm->p)) ) {
		case 65: goto st432;
		case 93: goto tr479;
		case 97: goto st484;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st484:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof484;
case 484:
	switch( (*( sm->p)) ) {
		case 82: goto st433;
		case 93: goto tr479;
		case 114: goto st485;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st485:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof485;
case 485:
	switch( (*( sm->p)) ) {
		case 65: goto st434;
		case 93: goto tr485;
		case 97: goto st486;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st486:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof486;
case 486:
	switch( (*( sm->p)) ) {
		case 67: goto st435;
		case 93: goto tr479;
		case 99: goto st487;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st487:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof487;
case 487:
	switch( (*( sm->p)) ) {
		case 84: goto st436;
		case 93: goto tr479;
		case 116: goto st488;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st488:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof488;
case 488:
	switch( (*( sm->p)) ) {
		case 69: goto st437;
		case 93: goto tr479;
		case 101: goto st489;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st489:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof489;
case 489:
	switch( (*( sm->p)) ) {
		case 82: goto st429;
		case 93: goto tr479;
		case 114: goto st481;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st490:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof490;
case 490:
	switch( (*( sm->p)) ) {
		case 78: goto st439;
		case 80: goto st446;
		case 93: goto tr479;
		case 110: goto st491;
		case 112: goto st498;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st491:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof491;
case 491:
	switch( (*( sm->p)) ) {
		case 84: goto st440;
		case 93: goto tr479;
		case 116: goto st492;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st492:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof492;
case 492:
	switch( (*( sm->p)) ) {
		case 82: goto st441;
		case 93: goto tr485;
		case 114: goto st493;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st493:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof493;
case 493:
	switch( (*( sm->p)) ) {
		case 73: goto st442;
		case 93: goto tr479;
		case 105: goto st494;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st494:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof494;
case 494:
	switch( (*( sm->p)) ) {
		case 66: goto st443;
		case 93: goto tr479;
		case 98: goto st495;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st495:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof495;
case 495:
	switch( (*( sm->p)) ) {
		case 85: goto st444;
		case 93: goto tr479;
		case 117: goto st496;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st496:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof496;
case 496:
	switch( (*( sm->p)) ) {
		case 84: goto st445;
		case 93: goto tr479;
		case 116: goto st497;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st497:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof497;
case 497:
	switch( (*( sm->p)) ) {
		case 79: goto st437;
		case 93: goto tr479;
		case 111: goto st489;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st498:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof498;
case 498:
	switch( (*( sm->p)) ) {
		case 89: goto st447;
		case 93: goto tr479;
		case 121: goto st499;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st499:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof499;
case 499:
	switch( (*( sm->p)) ) {
		case 82: goto st448;
		case 93: goto tr485;
		case 114: goto st500;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st500:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof500;
case 500:
	switch( (*( sm->p)) ) {
		case 73: goto st449;
		case 93: goto tr479;
		case 105: goto st501;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st501:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof501;
case 501:
	switch( (*( sm->p)) ) {
		case 71: goto st450;
		case 93: goto tr479;
		case 103: goto st502;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st502:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof502;
case 502:
	switch( (*( sm->p)) ) {
		case 72: goto st428;
		case 93: goto tr479;
		case 104: goto st480;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
tr470:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st503;
st503:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof503;
case 503:
#line 7854 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st452;
		case 93: goto tr479;
		case 101: goto st504;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st504:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof504;
case 504:
	switch( (*( sm->p)) ) {
		case 78: goto st453;
		case 93: goto tr479;
		case 110: goto st505;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st505:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof505;
case 505:
	switch( (*( sm->p)) ) {
		case 69: goto st454;
		case 93: goto tr485;
		case 101: goto st506;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st506:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof506;
case 506:
	switch( (*( sm->p)) ) {
		case 82: goto st455;
		case 93: goto tr479;
		case 114: goto st507;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st507:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof507;
case 507:
	switch( (*( sm->p)) ) {
		case 65: goto st456;
		case 93: goto tr479;
		case 97: goto st508;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st508:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof508;
case 508:
	switch( (*( sm->p)) ) {
		case 76: goto st429;
		case 93: goto tr479;
		case 108: goto st481;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
tr471:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st509;
st509:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof509;
case 509:
#line 7929 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st458;
		case 93: goto tr479;
		case 110: goto st510;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st510:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof510;
case 510:
	switch( (*( sm->p)) ) {
		case 86: goto st459;
		case 93: goto tr479;
		case 118: goto st511;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st511:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof511;
case 511:
	switch( (*( sm->p)) ) {
		case 65: goto st460;
		case 93: goto tr485;
		case 97: goto st512;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st512:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof512;
case 512:
	switch( (*( sm->p)) ) {
		case 76: goto st461;
		case 93: goto tr479;
		case 108: goto st513;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st513:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof513;
case 513:
	switch( (*( sm->p)) ) {
		case 73: goto st462;
		case 93: goto tr479;
		case 105: goto st514;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st514:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof514;
case 514:
	switch( (*( sm->p)) ) {
		case 68: goto st429;
		case 93: goto tr479;
		case 100: goto st481;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
tr472:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st515;
st515:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof515;
case 515:
#line 8004 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st464;
		case 93: goto tr479;
		case 111: goto st516;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st516:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof516;
case 516:
	switch( (*( sm->p)) ) {
		case 82: goto st465;
		case 93: goto tr479;
		case 114: goto st517;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st517:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof517;
case 517:
	switch( (*( sm->p)) ) {
		case 69: goto st429;
		case 93: goto tr485;
		case 101: goto st481;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
tr473:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st518;
st518:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof518;
case 518:
#line 8043 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st467;
		case 93: goto tr479;
		case 101: goto st519;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st519:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof519;
case 519:
	switch( (*( sm->p)) ) {
		case 84: goto st468;
		case 93: goto tr479;
		case 116: goto st520;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st520:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof520;
case 520:
	switch( (*( sm->p)) ) {
		case 65: goto st429;
		case 93: goto tr479;
		case 97: goto st481;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
tr474:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st521;
st521:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof521;
case 521:
#line 8082 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st470;
		case 93: goto tr479;
		case 112: goto st522;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st522:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof522;
case 522:
	switch( (*( sm->p)) ) {
		case 69: goto st471;
		case 93: goto tr479;
		case 101: goto st523;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st523:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof523;
case 523:
	switch( (*( sm->p)) ) {
		case 67: goto st472;
		case 93: goto tr479;
		case 99: goto st524;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st524:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof524;
case 524:
	switch( (*( sm->p)) ) {
		case 73: goto st473;
		case 93: goto tr485;
		case 105: goto st525;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st525:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof525;
case 525:
	switch( (*( sm->p)) ) {
		case 69: goto st474;
		case 93: goto tr479;
		case 101: goto st526;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st526:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof526;
case 526:
	switch( (*( sm->p)) ) {
		case 83: goto st429;
		case 93: goto tr479;
		case 115: goto st481;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st476;
	goto tr224;
st527:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof527;
case 527:
	if ( (*( sm->p)) == 93 )
		goto tr572;
	goto tr224;
st528:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof528;
case 528:
	switch( (*( sm->p)) ) {
		case 85: goto st529;
		case 117: goto st529;
	}
	goto tr224;
st529:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof529;
case 529:
	switch( (*( sm->p)) ) {
		case 79: goto st530;
		case 111: goto st530;
	}
	goto tr224;
st530:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof530;
case 530:
	switch( (*( sm->p)) ) {
		case 84: goto st531;
		case 116: goto st531;
	}
	goto tr224;
st531:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof531;
case 531:
	switch( (*( sm->p)) ) {
		case 69: goto st532;
		case 101: goto st532;
	}
	goto tr224;
st532:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof532;
case 532:
	switch( (*( sm->p)) ) {
		case 61: goto st533;
		case 93: goto tr578;
	}
	goto tr224;
st533:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof533;
case 533:
	switch( (*( sm->p)) ) {
		case 35: goto tr579;
		case 65: goto tr580;
		case 67: goto tr581;
		case 71: goto tr582;
		case 73: goto tr583;
		case 76: goto tr584;
		case 77: goto tr585;
		case 83: goto tr586;
		case 97: goto tr587;
		case 99: goto tr589;
		case 103: goto tr590;
		case 105: goto tr591;
		case 108: goto tr592;
		case 109: goto tr593;
		case 115: goto tr594;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto tr588;
	goto tr224;
tr579:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st534;
st534:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof534;
case 534:
#line 8233 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st535;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st535;
	} else
		goto st535;
	goto tr224;
st535:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof535;
case 535:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st536;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st536;
	} else
		goto st536;
	goto tr224;
st536:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof536;
case 536:
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st537;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st537;
	} else
		goto st537;
	goto tr224;
st537:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof537;
case 537:
	if ( (*( sm->p)) == 93 )
		goto tr599;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st538;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st538;
	} else
		goto st538;
	goto tr224;
st538:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof538;
case 538:
	if ( (*( sm->p)) == 93 )
		goto tr599;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st539;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st539;
	} else
		goto st539;
	goto tr224;
st539:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof539;
case 539:
	if ( (*( sm->p)) == 93 )
		goto tr599;
	if ( (*( sm->p)) < 65 ) {
		if ( 48 <= (*( sm->p)) && (*( sm->p)) <= 57 )
			goto st540;
	} else if ( (*( sm->p)) > 70 ) {
		if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 102 )
			goto st540;
	} else
		goto st540;
	goto tr224;
st540:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof540;
case 540:
	if ( (*( sm->p)) == 93 )
		goto tr599;
	goto tr224;
tr580:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st541;
st541:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof541;
case 541:
#line 8327 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st542;
		case 114: goto st542;
	}
	goto tr224;
st542:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof542;
case 542:
	switch( (*( sm->p)) ) {
		case 84: goto st543;
		case 116: goto st543;
	}
	goto tr224;
st543:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof543;
case 543:
	switch( (*( sm->p)) ) {
		case 73: goto st544;
		case 93: goto tr605;
		case 105: goto st544;
	}
	goto tr224;
st544:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof544;
case 544:
	switch( (*( sm->p)) ) {
		case 83: goto st545;
		case 115: goto st545;
	}
	goto tr224;
st545:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof545;
case 545:
	switch( (*( sm->p)) ) {
		case 84: goto st546;
		case 116: goto st546;
	}
	goto tr224;
st546:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof546;
case 546:
	if ( (*( sm->p)) == 93 )
		goto tr605;
	goto tr224;
tr581:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st547;
st547:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof547;
case 547:
#line 8383 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st548;
		case 79: goto st555;
		case 104: goto st548;
		case 111: goto st555;
	}
	goto tr224;
st548:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof548;
case 548:
	switch( (*( sm->p)) ) {
		case 65: goto st549;
		case 97: goto st549;
	}
	goto tr224;
st549:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof549;
case 549:
	switch( (*( sm->p)) ) {
		case 82: goto st550;
		case 114: goto st550;
	}
	goto tr224;
st550:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof550;
case 550:
	switch( (*( sm->p)) ) {
		case 65: goto st551;
		case 93: goto tr605;
		case 97: goto st551;
	}
	goto tr224;
st551:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof551;
case 551:
	switch( (*( sm->p)) ) {
		case 67: goto st552;
		case 99: goto st552;
	}
	goto tr224;
st552:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof552;
case 552:
	switch( (*( sm->p)) ) {
		case 84: goto st553;
		case 116: goto st553;
	}
	goto tr224;
st553:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof553;
case 553:
	switch( (*( sm->p)) ) {
		case 69: goto st554;
		case 101: goto st554;
	}
	goto tr224;
st554:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof554;
case 554:
	switch( (*( sm->p)) ) {
		case 82: goto st546;
		case 114: goto st546;
	}
	goto tr224;
st555:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof555;
case 555:
	switch( (*( sm->p)) ) {
		case 78: goto st556;
		case 80: goto st563;
		case 110: goto st556;
		case 112: goto st563;
	}
	goto tr224;
st556:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof556;
case 556:
	switch( (*( sm->p)) ) {
		case 84: goto st557;
		case 116: goto st557;
	}
	goto tr224;
st557:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof557;
case 557:
	switch( (*( sm->p)) ) {
		case 82: goto st558;
		case 93: goto tr605;
		case 114: goto st558;
	}
	goto tr224;
st558:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof558;
case 558:
	switch( (*( sm->p)) ) {
		case 73: goto st559;
		case 105: goto st559;
	}
	goto tr224;
st559:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof559;
case 559:
	switch( (*( sm->p)) ) {
		case 66: goto st560;
		case 98: goto st560;
	}
	goto tr224;
st560:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof560;
case 560:
	switch( (*( sm->p)) ) {
		case 85: goto st561;
		case 117: goto st561;
	}
	goto tr224;
st561:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof561;
case 561:
	switch( (*( sm->p)) ) {
		case 84: goto st562;
		case 116: goto st562;
	}
	goto tr224;
st562:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof562;
case 562:
	switch( (*( sm->p)) ) {
		case 79: goto st554;
		case 111: goto st554;
	}
	goto tr224;
st563:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof563;
case 563:
	switch( (*( sm->p)) ) {
		case 89: goto st564;
		case 121: goto st564;
	}
	goto tr224;
st564:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof564;
case 564:
	switch( (*( sm->p)) ) {
		case 82: goto st565;
		case 93: goto tr605;
		case 114: goto st565;
	}
	goto tr224;
st565:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof565;
case 565:
	switch( (*( sm->p)) ) {
		case 73: goto st566;
		case 105: goto st566;
	}
	goto tr224;
st566:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof566;
case 566:
	switch( (*( sm->p)) ) {
		case 71: goto st567;
		case 103: goto st567;
	}
	goto tr224;
st567:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof567;
case 567:
	switch( (*( sm->p)) ) {
		case 72: goto st545;
		case 104: goto st545;
	}
	goto tr224;
tr582:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st568;
st568:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof568;
case 568:
#line 8582 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st569;
		case 101: goto st569;
	}
	goto tr224;
st569:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof569;
case 569:
	switch( (*( sm->p)) ) {
		case 78: goto st570;
		case 110: goto st570;
	}
	goto tr224;
st570:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof570;
case 570:
	switch( (*( sm->p)) ) {
		case 69: goto st571;
		case 93: goto tr605;
		case 101: goto st571;
	}
	goto tr224;
st571:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof571;
case 571:
	switch( (*( sm->p)) ) {
		case 82: goto st572;
		case 114: goto st572;
	}
	goto tr224;
st572:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof572;
case 572:
	switch( (*( sm->p)) ) {
		case 65: goto st573;
		case 97: goto st573;
	}
	goto tr224;
st573:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof573;
case 573:
	switch( (*( sm->p)) ) {
		case 76: goto st546;
		case 108: goto st546;
	}
	goto tr224;
tr583:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st574;
st574:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof574;
case 574:
#line 8640 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st575;
		case 110: goto st575;
	}
	goto tr224;
st575:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof575;
case 575:
	switch( (*( sm->p)) ) {
		case 86: goto st576;
		case 118: goto st576;
	}
	goto tr224;
st576:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof576;
case 576:
	switch( (*( sm->p)) ) {
		case 65: goto st577;
		case 93: goto tr605;
		case 97: goto st577;
	}
	goto tr224;
st577:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof577;
case 577:
	switch( (*( sm->p)) ) {
		case 76: goto st578;
		case 108: goto st578;
	}
	goto tr224;
st578:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof578;
case 578:
	switch( (*( sm->p)) ) {
		case 73: goto st579;
		case 105: goto st579;
	}
	goto tr224;
st579:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof579;
case 579:
	switch( (*( sm->p)) ) {
		case 68: goto st546;
		case 100: goto st546;
	}
	goto tr224;
tr584:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st580;
st580:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof580;
case 580:
#line 8698 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st581;
		case 111: goto st581;
	}
	goto tr224;
st581:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof581;
case 581:
	switch( (*( sm->p)) ) {
		case 82: goto st582;
		case 114: goto st582;
	}
	goto tr224;
st582:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof582;
case 582:
	switch( (*( sm->p)) ) {
		case 69: goto st546;
		case 93: goto tr605;
		case 101: goto st546;
	}
	goto tr224;
tr585:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st583;
st583:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof583;
case 583:
#line 8729 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st584;
		case 101: goto st584;
	}
	goto tr224;
st584:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof584;
case 584:
	switch( (*( sm->p)) ) {
		case 84: goto st585;
		case 116: goto st585;
	}
	goto tr224;
st585:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof585;
case 585:
	switch( (*( sm->p)) ) {
		case 65: goto st546;
		case 97: goto st546;
	}
	goto tr224;
tr586:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st586;
st586:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof586;
case 586:
#line 8759 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st587;
		case 112: goto st587;
	}
	goto tr224;
st587:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof587;
case 587:
	switch( (*( sm->p)) ) {
		case 69: goto st588;
		case 101: goto st588;
	}
	goto tr224;
st588:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof588;
case 588:
	switch( (*( sm->p)) ) {
		case 67: goto st589;
		case 99: goto st589;
	}
	goto tr224;
st589:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof589;
case 589:
	switch( (*( sm->p)) ) {
		case 73: goto st590;
		case 93: goto tr605;
		case 105: goto st590;
	}
	goto tr224;
st590:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof590;
case 590:
	switch( (*( sm->p)) ) {
		case 69: goto st591;
		case 101: goto st591;
	}
	goto tr224;
st591:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof591;
case 591:
	switch( (*( sm->p)) ) {
		case 83: goto st546;
		case 115: goto st546;
	}
	goto tr224;
tr587:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st592;
st592:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof592;
case 592:
#line 8817 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 82: goto st542;
		case 93: goto tr599;
		case 114: goto st594;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
tr588:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st593;
st593:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof593;
case 593:
#line 8832 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr599;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st594:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof594;
case 594:
	switch( (*( sm->p)) ) {
		case 84: goto st543;
		case 93: goto tr599;
		case 116: goto st595;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st595:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof595;
case 595:
	switch( (*( sm->p)) ) {
		case 73: goto st544;
		case 93: goto tr599;
		case 105: goto st596;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st596:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof596;
case 596:
	switch( (*( sm->p)) ) {
		case 83: goto st545;
		case 93: goto tr599;
		case 115: goto st597;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st597:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof597;
case 597:
	switch( (*( sm->p)) ) {
		case 84: goto st546;
		case 93: goto tr599;
		case 116: goto st598;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st598:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof598;
case 598:
	if ( (*( sm->p)) == 93 )
		goto tr599;
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
tr589:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st599;
st599:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof599;
case 599:
#line 8901 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 72: goto st548;
		case 79: goto st555;
		case 93: goto tr599;
		case 104: goto st600;
		case 111: goto st607;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st600:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof600;
case 600:
	switch( (*( sm->p)) ) {
		case 65: goto st549;
		case 93: goto tr599;
		case 97: goto st601;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st601:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof601;
case 601:
	switch( (*( sm->p)) ) {
		case 82: goto st550;
		case 93: goto tr599;
		case 114: goto st602;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st602:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof602;
case 602:
	switch( (*( sm->p)) ) {
		case 65: goto st551;
		case 93: goto tr599;
		case 97: goto st603;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st603:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof603;
case 603:
	switch( (*( sm->p)) ) {
		case 67: goto st552;
		case 93: goto tr599;
		case 99: goto st604;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st604:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof604;
case 604:
	switch( (*( sm->p)) ) {
		case 84: goto st553;
		case 93: goto tr599;
		case 116: goto st605;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st605:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof605;
case 605:
	switch( (*( sm->p)) ) {
		case 69: goto st554;
		case 93: goto tr599;
		case 101: goto st606;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st606:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof606;
case 606:
	switch( (*( sm->p)) ) {
		case 82: goto st546;
		case 93: goto tr599;
		case 114: goto st598;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st607:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof607;
case 607:
	switch( (*( sm->p)) ) {
		case 78: goto st556;
		case 80: goto st563;
		case 93: goto tr599;
		case 110: goto st608;
		case 112: goto st615;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st608:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof608;
case 608:
	switch( (*( sm->p)) ) {
		case 84: goto st557;
		case 93: goto tr599;
		case 116: goto st609;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st609:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof609;
case 609:
	switch( (*( sm->p)) ) {
		case 82: goto st558;
		case 93: goto tr599;
		case 114: goto st610;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st610:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof610;
case 610:
	switch( (*( sm->p)) ) {
		case 73: goto st559;
		case 93: goto tr599;
		case 105: goto st611;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st611:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof611;
case 611:
	switch( (*( sm->p)) ) {
		case 66: goto st560;
		case 93: goto tr599;
		case 98: goto st612;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st612:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof612;
case 612:
	switch( (*( sm->p)) ) {
		case 85: goto st561;
		case 93: goto tr599;
		case 117: goto st613;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st613:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof613;
case 613:
	switch( (*( sm->p)) ) {
		case 84: goto st562;
		case 93: goto tr599;
		case 116: goto st614;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st614:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof614;
case 614:
	switch( (*( sm->p)) ) {
		case 79: goto st554;
		case 93: goto tr599;
		case 111: goto st606;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st615:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof615;
case 615:
	switch( (*( sm->p)) ) {
		case 89: goto st564;
		case 93: goto tr599;
		case 121: goto st616;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st616:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof616;
case 616:
	switch( (*( sm->p)) ) {
		case 82: goto st565;
		case 93: goto tr599;
		case 114: goto st617;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st617:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof617;
case 617:
	switch( (*( sm->p)) ) {
		case 73: goto st566;
		case 93: goto tr599;
		case 105: goto st618;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st618:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof618;
case 618:
	switch( (*( sm->p)) ) {
		case 71: goto st567;
		case 93: goto tr599;
		case 103: goto st619;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st619:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof619;
case 619:
	switch( (*( sm->p)) ) {
		case 72: goto st545;
		case 93: goto tr599;
		case 104: goto st597;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
tr590:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st620;
st620:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof620;
case 620:
#line 9160 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st569;
		case 93: goto tr599;
		case 101: goto st621;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st621:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof621;
case 621:
	switch( (*( sm->p)) ) {
		case 78: goto st570;
		case 93: goto tr599;
		case 110: goto st622;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st622:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof622;
case 622:
	switch( (*( sm->p)) ) {
		case 69: goto st571;
		case 93: goto tr599;
		case 101: goto st623;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st623:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof623;
case 623:
	switch( (*( sm->p)) ) {
		case 82: goto st572;
		case 93: goto tr599;
		case 114: goto st624;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st624:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof624;
case 624:
	switch( (*( sm->p)) ) {
		case 65: goto st573;
		case 93: goto tr599;
		case 97: goto st625;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st625:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof625;
case 625:
	switch( (*( sm->p)) ) {
		case 76: goto st546;
		case 93: goto tr599;
		case 108: goto st598;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
tr591:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st626;
st626:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof626;
case 626:
#line 9235 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 78: goto st575;
		case 93: goto tr599;
		case 110: goto st627;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st627:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof627;
case 627:
	switch( (*( sm->p)) ) {
		case 86: goto st576;
		case 93: goto tr599;
		case 118: goto st628;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st628:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof628;
case 628:
	switch( (*( sm->p)) ) {
		case 65: goto st577;
		case 93: goto tr599;
		case 97: goto st629;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st629:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof629;
case 629:
	switch( (*( sm->p)) ) {
		case 76: goto st578;
		case 93: goto tr599;
		case 108: goto st630;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st630:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof630;
case 630:
	switch( (*( sm->p)) ) {
		case 73: goto st579;
		case 93: goto tr599;
		case 105: goto st631;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st631:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof631;
case 631:
	switch( (*( sm->p)) ) {
		case 68: goto st546;
		case 93: goto tr599;
		case 100: goto st598;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
tr592:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st632;
st632:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof632;
case 632:
#line 9310 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 79: goto st581;
		case 93: goto tr599;
		case 111: goto st633;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st633:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof633;
case 633:
	switch( (*( sm->p)) ) {
		case 82: goto st582;
		case 93: goto tr599;
		case 114: goto st634;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st634:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof634;
case 634:
	switch( (*( sm->p)) ) {
		case 69: goto st546;
		case 93: goto tr599;
		case 101: goto st598;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
tr593:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st635;
st635:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof635;
case 635:
#line 9349 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 69: goto st584;
		case 93: goto tr599;
		case 101: goto st636;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st636:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof636;
case 636:
	switch( (*( sm->p)) ) {
		case 84: goto st585;
		case 93: goto tr599;
		case 116: goto st637;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st637:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof637;
case 637:
	switch( (*( sm->p)) ) {
		case 65: goto st546;
		case 93: goto tr599;
		case 97: goto st598;
	}
	if ( 98 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
tr594:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st638;
st638:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof638;
case 638:
#line 9388 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 80: goto st587;
		case 93: goto tr599;
		case 112: goto st639;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st639:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof639;
case 639:
	switch( (*( sm->p)) ) {
		case 69: goto st588;
		case 93: goto tr599;
		case 101: goto st640;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st640:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof640;
case 640:
	switch( (*( sm->p)) ) {
		case 67: goto st589;
		case 93: goto tr599;
		case 99: goto st641;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st641:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof641;
case 641:
	switch( (*( sm->p)) ) {
		case 73: goto st590;
		case 93: goto tr599;
		case 105: goto st642;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st642:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof642;
case 642:
	switch( (*( sm->p)) ) {
		case 69: goto st591;
		case 93: goto tr599;
		case 101: goto st643;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st643:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof643;
case 643:
	switch( (*( sm->p)) ) {
		case 83: goto st546;
		case 93: goto tr599;
		case 115: goto st598;
	}
	if ( 97 <= (*( sm->p)) && (*( sm->p)) <= 122 )
		goto st593;
	goto tr224;
st644:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof644;
case 644:
	switch( (*( sm->p)) ) {
		case 69: goto st645;
		case 80: goto st652;
		case 85: goto st659;
		case 93: goto tr695;
		case 101: goto st645;
		case 112: goto st652;
		case 117: goto st659;
	}
	goto tr224;
st645:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof645;
case 645:
	switch( (*( sm->p)) ) {
		case 67: goto st646;
		case 99: goto st646;
	}
	goto tr224;
st646:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof646;
case 646:
	switch( (*( sm->p)) ) {
		case 84: goto st647;
		case 116: goto st647;
	}
	goto tr224;
st647:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof647;
case 647:
	switch( (*( sm->p)) ) {
		case 73: goto st648;
		case 105: goto st648;
	}
	goto tr224;
st648:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof648;
case 648:
	switch( (*( sm->p)) ) {
		case 79: goto st649;
		case 111: goto st649;
	}
	goto tr224;
st649:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof649;
case 649:
	switch( (*( sm->p)) ) {
		case 78: goto st650;
		case 110: goto st650;
	}
	goto tr224;
st650:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof650;
case 650:
	switch( (*( sm->p)) ) {
		case 44: goto tr701;
		case 61: goto tr701;
		case 93: goto tr702;
	}
	goto tr224;
tr701:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st651;
st651:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof651;
case 651:
#line 9532 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr704;
	goto st651;
st652:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof652;
case 652:
	switch( (*( sm->p)) ) {
		case 79: goto st653;
		case 111: goto st653;
	}
	goto tr224;
st653:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof653;
case 653:
	switch( (*( sm->p)) ) {
		case 73: goto st654;
		case 105: goto st654;
	}
	goto tr224;
st654:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof654;
case 654:
	switch( (*( sm->p)) ) {
		case 76: goto st655;
		case 108: goto st655;
	}
	goto tr224;
st655:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof655;
case 655:
	switch( (*( sm->p)) ) {
		case 69: goto st656;
		case 101: goto st656;
	}
	goto tr224;
st656:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof656;
case 656:
	switch( (*( sm->p)) ) {
		case 82: goto st657;
		case 114: goto st657;
	}
	goto tr224;
st657:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof657;
case 657:
	switch( (*( sm->p)) ) {
		case 83: goto st658;
		case 93: goto tr711;
		case 115: goto st658;
	}
	goto tr224;
st658:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof658;
case 658:
	if ( (*( sm->p)) == 93 )
		goto tr711;
	goto tr224;
st659:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof659;
case 659:
	switch( (*( sm->p)) ) {
		case 66: goto st660;
		case 80: goto st661;
		case 98: goto st660;
		case 112: goto st661;
	}
	goto tr224;
st660:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof660;
case 660:
	if ( (*( sm->p)) == 93 )
		goto tr714;
	goto tr224;
st661:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof661;
case 661:
	if ( (*( sm->p)) == 93 )
		goto tr715;
	goto tr224;
st662:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof662;
case 662:
	switch( (*( sm->p)) ) {
		case 65: goto st663;
		case 97: goto st663;
	}
	goto tr224;
st663:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof663;
case 663:
	switch( (*( sm->p)) ) {
		case 66: goto st664;
		case 98: goto st664;
	}
	goto tr224;
st664:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof664;
case 664:
	switch( (*( sm->p)) ) {
		case 76: goto st665;
		case 108: goto st665;
	}
	goto tr224;
st665:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof665;
case 665:
	switch( (*( sm->p)) ) {
		case 69: goto st666;
		case 101: goto st666;
	}
	goto tr224;
st666:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof666;
case 666:
	if ( (*( sm->p)) == 93 )
		goto tr720;
	goto tr224;
st667:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof667;
case 667:
	if ( (*( sm->p)) == 93 )
		goto tr721;
	goto tr224;
st668:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof668;
case 668:
	switch( (*( sm->p)) ) {
		case 93: goto tr224;
		case 124: goto tr723;
	}
	goto tr722;
tr722:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st669;
st669:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof669;
case 669:
#line 9688 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 93: goto tr725;
		case 124: goto tr726;
	}
	goto st669;
tr725:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st670;
st670:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof670;
case 670:
#line 9700 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr727;
	goto tr224;
tr726:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st671;
st671:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof671;
case 671:
#line 9710 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 93: goto tr224;
		case 124: goto tr224;
	}
	goto tr728;
tr728:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st672;
st672:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof672;
case 672:
#line 9722 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 93: goto tr730;
		case 124: goto tr224;
	}
	goto st672;
tr730:
#line 96 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
	goto st673;
st673:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof673;
case 673:
#line 9734 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 93 )
		goto tr731;
	goto tr224;
tr723:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st674;
st674:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof674;
case 674:
#line 9744 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 93: goto tr725;
		case 124: goto tr224;
	}
	goto st674;
st781:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof781;
case 781:
	if ( (*( sm->p)) == 96 )
		goto tr936;
	goto tr854;
tr839:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st782;
st782:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof782;
case 782:
#line 9763 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 123 )
		goto st675;
	goto tr854;
st675:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof675;
case 675:
	switch( (*( sm->p)) ) {
		case 124: goto tr734;
		case 125: goto tr224;
	}
	goto tr733;
tr733:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st676;
st676:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof676;
case 676:
#line 9782 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 124: goto tr736;
		case 125: goto tr737;
	}
	goto st676;
tr736:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st677;
st677:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof677;
case 677:
#line 9794 "ext/dtext/dtext.cpp"
	if ( 124 <= (*( sm->p)) && (*( sm->p)) <= 125 )
		goto tr224;
	goto tr738;
tr738:
#line 95 "ext/dtext/dtext.cpp.rl"
	{ b1 = p; }
	goto st678;
st678:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof678;
case 678:
#line 9804 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 124: goto tr224;
		case 125: goto tr740;
	}
	goto st678;
tr740:
#line 96 "ext/dtext/dtext.cpp.rl"
	{ b2 = p; }
	goto st679;
st679:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof679;
case 679:
#line 9816 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 125 )
		goto tr741;
	goto tr224;
tr737:
#line 94 "ext/dtext/dtext.cpp.rl"
	{ a2 = p; }
	goto st680;
st680:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof680;
case 680:
#line 9826 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 125 )
		goto tr742;
	goto tr224;
tr734:
#line 93 "ext/dtext/dtext.cpp.rl"
	{ a1 = p; }
	goto st681;
st681:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof681;
case 681:
#line 9836 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 124: goto tr224;
		case 125: goto tr737;
	}
	goto st681;
tr938:
#line 532 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_html_escaped((*( sm->p)));
  }}
	goto st783;
tr940:
#line 527 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append("</span>");
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st783;
tr941:
#line 532 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    append_html_escaped((*( sm->p)));
  }}
	goto st783;
tr942:
#line 523 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append("`");
  }}
	goto st783;
st783:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof783;
case 783:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 9868 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 92: goto st784;
		case 96: goto tr940;
	}
	goto tr938;
st784:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof784;
case 784:
	if ( (*( sm->p)) == 96 )
		goto tr942;
	goto tr941;
tr744:
#line 547 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}{
    append_html_escaped((*( sm->p)));
  }}
	goto st785;
tr749:
#line 538 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_check(BLOCK_CODE)) {
      dstack_rewind();
    } else {
      append("[/code]");
    }
    { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
  }}
	goto st785;
tr943:
#line 547 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    append_html_escaped((*( sm->p)));
  }}
	goto st785;
tr945:
#line 547 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;{
    append_html_escaped((*( sm->p)));
  }}
	goto st785;
st785:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof785;
case 785:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 9911 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 91 )
		goto tr944;
	goto tr943;
tr944:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st786;
st786:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof786;
case 786:
#line 9921 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 47 )
		goto st682;
	goto tr945;
st682:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof682;
case 682:
	switch( (*( sm->p)) ) {
		case 67: goto st683;
		case 99: goto st683;
	}
	goto tr744;
st683:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof683;
case 683:
	switch( (*( sm->p)) ) {
		case 79: goto st684;
		case 111: goto st684;
	}
	goto tr744;
st684:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof684;
case 684:
	switch( (*( sm->p)) ) {
		case 68: goto st685;
		case 100: goto st685;
	}
	goto tr744;
st685:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof685;
case 685:
	switch( (*( sm->p)) ) {
		case 69: goto st686;
		case 101: goto st686;
	}
	goto tr744;
st686:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof686;
case 686:
	if ( (*( sm->p)) == 93 )
		goto tr749;
	goto tr744;
tr750:
#line 593 "ext/dtext/dtext.cpp.rl"
	{{( sm->p) = ((( sm->te)))-1;}}
	goto st787;
tr759:
#line 587 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    if (dstack_close_block(BLOCK_TABLE, "</table>")) {
      { sm->cs = ( (sm->stack.data()))[--( sm->top)];goto _again;}
    }
  }}
	goto st787;
tr763:
#line 565 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_block(BLOCK_TBODY, "</tbody>");
  }}
	goto st787;
tr767:
#line 557 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_block(BLOCK_THEAD, "</thead>");
  }}
	goto st787;
tr768:
#line 578 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_close_block(BLOCK_TR, "</tr>");
  }}
	goto st787;
tr776:
#line 561 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_block(BLOCK_TBODY, "<tbody>");
  }}
	goto st787;
tr777:
#line 582 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_block(BLOCK_TD, "<td>");
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 787;goto st729;}}
  }}
	goto st787;
tr779:
#line 569 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_block(BLOCK_TH, "<th>");
    {
  size_t len = stack.size();

  // Should never happen.
  if (len > MAX_STACK_DEPTH) {
    throw DTextError("too many nested elements");
  }

  if (top >= len) {
    g_debug("growing stack %zi\n", len + 16);
    stack.resize(len + 16, 0);
  }
{( (sm->stack.data()))[( sm->top)++] = 787;goto st729;}}
  }}
	goto st787;
tr782:
#line 553 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_block(BLOCK_THEAD, "<thead>");
  }}
	goto st787;
tr783:
#line 574 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;{
    dstack_open_block(BLOCK_TR, "<tr>");
  }}
	goto st787;
tr947:
#line 593 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p)+1;}
	goto st787;
tr949:
#line 593 "ext/dtext/dtext.cpp.rl"
	{( sm->te) = ( sm->p);( sm->p)--;}
	goto st787;
st787:
#line 1 "NONE"
	{( sm->ts) = 0;}
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof787;
case 787:
#line 1 "NONE"
	{( sm->ts) = ( sm->p);}
#line 10055 "ext/dtext/dtext.cpp"
	if ( (*( sm->p)) == 91 )
		goto tr948;
	goto tr947;
tr948:
#line 1 "NONE"
	{( sm->te) = ( sm->p)+1;}
	goto st788;
st788:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof788;
case 788:
#line 10065 "ext/dtext/dtext.cpp"
	switch( (*( sm->p)) ) {
		case 47: goto st687;
		case 84: goto st702;
		case 116: goto st702;
	}
	goto tr949;
st687:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof687;
case 687:
	switch( (*( sm->p)) ) {
		case 84: goto st688;
		case 116: goto st688;
	}
	goto tr750;
st688:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof688;
case 688:
	switch( (*( sm->p)) ) {
		case 65: goto st689;
		case 66: goto st693;
		case 72: goto st697;
		case 82: goto st701;
		case 97: goto st689;
		case 98: goto st693;
		case 104: goto st697;
		case 114: goto st701;
	}
	goto tr750;
st689:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof689;
case 689:
	switch( (*( sm->p)) ) {
		case 66: goto st690;
		case 98: goto st690;
	}
	goto tr750;
st690:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof690;
case 690:
	switch( (*( sm->p)) ) {
		case 76: goto st691;
		case 108: goto st691;
	}
	goto tr750;
st691:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof691;
case 691:
	switch( (*( sm->p)) ) {
		case 69: goto st692;
		case 101: goto st692;
	}
	goto tr750;
st692:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof692;
case 692:
	if ( (*( sm->p)) == 93 )
		goto tr759;
	goto tr750;
st693:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof693;
case 693:
	switch( (*( sm->p)) ) {
		case 79: goto st694;
		case 111: goto st694;
	}
	goto tr750;
st694:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof694;
case 694:
	switch( (*( sm->p)) ) {
		case 68: goto st695;
		case 100: goto st695;
	}
	goto tr750;
st695:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof695;
case 695:
	switch( (*( sm->p)) ) {
		case 89: goto st696;
		case 121: goto st696;
	}
	goto tr750;
st696:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof696;
case 696:
	if ( (*( sm->p)) == 93 )
		goto tr763;
	goto tr750;
st697:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof697;
case 697:
	switch( (*( sm->p)) ) {
		case 69: goto st698;
		case 101: goto st698;
	}
	goto tr750;
st698:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof698;
case 698:
	switch( (*( sm->p)) ) {
		case 65: goto st699;
		case 97: goto st699;
	}
	goto tr750;
st699:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof699;
case 699:
	switch( (*( sm->p)) ) {
		case 68: goto st700;
		case 100: goto st700;
	}
	goto tr750;
st700:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof700;
case 700:
	if ( (*( sm->p)) == 93 )
		goto tr767;
	goto tr750;
st701:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof701;
case 701:
	if ( (*( sm->p)) == 93 )
		goto tr768;
	goto tr750;
st702:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof702;
case 702:
	switch( (*( sm->p)) ) {
		case 66: goto st703;
		case 68: goto st707;
		case 72: goto st708;
		case 82: goto st712;
		case 98: goto st703;
		case 100: goto st707;
		case 104: goto st708;
		case 114: goto st712;
	}
	goto tr750;
st703:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof703;
case 703:
	switch( (*( sm->p)) ) {
		case 79: goto st704;
		case 111: goto st704;
	}
	goto tr750;
st704:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof704;
case 704:
	switch( (*( sm->p)) ) {
		case 68: goto st705;
		case 100: goto st705;
	}
	goto tr750;
st705:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof705;
case 705:
	switch( (*( sm->p)) ) {
		case 89: goto st706;
		case 121: goto st706;
	}
	goto tr750;
st706:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof706;
case 706:
	if ( (*( sm->p)) == 93 )
		goto tr776;
	goto tr750;
st707:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof707;
case 707:
	if ( (*( sm->p)) == 93 )
		goto tr777;
	goto tr750;
st708:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof708;
case 708:
	switch( (*( sm->p)) ) {
		case 69: goto st709;
		case 93: goto tr779;
		case 101: goto st709;
	}
	goto tr750;
st709:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof709;
case 709:
	switch( (*( sm->p)) ) {
		case 65: goto st710;
		case 97: goto st710;
	}
	goto tr750;
st710:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof710;
case 710:
	switch( (*( sm->p)) ) {
		case 68: goto st711;
		case 100: goto st711;
	}
	goto tr750;
st711:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof711;
case 711:
	if ( (*( sm->p)) == 93 )
		goto tr782;
	goto tr750;
st712:
	if ( ++( sm->p) == ( sm->pe) )
		goto _test_eof712;
case 712:
	if ( (*( sm->p)) == 93 )
		goto tr783;
	goto tr750;
	}
	_test_eof713:  sm->cs = 713; goto _test_eof; 
	_test_eof714:  sm->cs = 714; goto _test_eof; 
	_test_eof0:  sm->cs = 0; goto _test_eof; 
	_test_eof715:  sm->cs = 715; goto _test_eof; 
	_test_eof716:  sm->cs = 716; goto _test_eof; 
	_test_eof1:  sm->cs = 1; goto _test_eof; 
	_test_eof717:  sm->cs = 717; goto _test_eof; 
	_test_eof718:  sm->cs = 718; goto _test_eof; 
	_test_eof2:  sm->cs = 2; goto _test_eof; 
	_test_eof719:  sm->cs = 719; goto _test_eof; 
	_test_eof3:  sm->cs = 3; goto _test_eof; 
	_test_eof720:  sm->cs = 720; goto _test_eof; 
	_test_eof721:  sm->cs = 721; goto _test_eof; 
	_test_eof4:  sm->cs = 4; goto _test_eof; 
	_test_eof5:  sm->cs = 5; goto _test_eof; 
	_test_eof6:  sm->cs = 6; goto _test_eof; 
	_test_eof7:  sm->cs = 7; goto _test_eof; 
	_test_eof8:  sm->cs = 8; goto _test_eof; 
	_test_eof9:  sm->cs = 9; goto _test_eof; 
	_test_eof10:  sm->cs = 10; goto _test_eof; 
	_test_eof11:  sm->cs = 11; goto _test_eof; 
	_test_eof12:  sm->cs = 12; goto _test_eof; 
	_test_eof13:  sm->cs = 13; goto _test_eof; 
	_test_eof14:  sm->cs = 14; goto _test_eof; 
	_test_eof15:  sm->cs = 15; goto _test_eof; 
	_test_eof16:  sm->cs = 16; goto _test_eof; 
	_test_eof722:  sm->cs = 722; goto _test_eof; 
	_test_eof17:  sm->cs = 17; goto _test_eof; 
	_test_eof18:  sm->cs = 18; goto _test_eof; 
	_test_eof19:  sm->cs = 19; goto _test_eof; 
	_test_eof20:  sm->cs = 20; goto _test_eof; 
	_test_eof21:  sm->cs = 21; goto _test_eof; 
	_test_eof22:  sm->cs = 22; goto _test_eof; 
	_test_eof23:  sm->cs = 23; goto _test_eof; 
	_test_eof24:  sm->cs = 24; goto _test_eof; 
	_test_eof25:  sm->cs = 25; goto _test_eof; 
	_test_eof26:  sm->cs = 26; goto _test_eof; 
	_test_eof27:  sm->cs = 27; goto _test_eof; 
	_test_eof28:  sm->cs = 28; goto _test_eof; 
	_test_eof29:  sm->cs = 29; goto _test_eof; 
	_test_eof30:  sm->cs = 30; goto _test_eof; 
	_test_eof31:  sm->cs = 31; goto _test_eof; 
	_test_eof32:  sm->cs = 32; goto _test_eof; 
	_test_eof33:  sm->cs = 33; goto _test_eof; 
	_test_eof34:  sm->cs = 34; goto _test_eof; 
	_test_eof35:  sm->cs = 35; goto _test_eof; 
	_test_eof36:  sm->cs = 36; goto _test_eof; 
	_test_eof37:  sm->cs = 37; goto _test_eof; 
	_test_eof38:  sm->cs = 38; goto _test_eof; 
	_test_eof39:  sm->cs = 39; goto _test_eof; 
	_test_eof40:  sm->cs = 40; goto _test_eof; 
	_test_eof41:  sm->cs = 41; goto _test_eof; 
	_test_eof42:  sm->cs = 42; goto _test_eof; 
	_test_eof43:  sm->cs = 43; goto _test_eof; 
	_test_eof44:  sm->cs = 44; goto _test_eof; 
	_test_eof45:  sm->cs = 45; goto _test_eof; 
	_test_eof46:  sm->cs = 46; goto _test_eof; 
	_test_eof47:  sm->cs = 47; goto _test_eof; 
	_test_eof48:  sm->cs = 48; goto _test_eof; 
	_test_eof49:  sm->cs = 49; goto _test_eof; 
	_test_eof50:  sm->cs = 50; goto _test_eof; 
	_test_eof51:  sm->cs = 51; goto _test_eof; 
	_test_eof52:  sm->cs = 52; goto _test_eof; 
	_test_eof53:  sm->cs = 53; goto _test_eof; 
	_test_eof54:  sm->cs = 54; goto _test_eof; 
	_test_eof55:  sm->cs = 55; goto _test_eof; 
	_test_eof56:  sm->cs = 56; goto _test_eof; 
	_test_eof57:  sm->cs = 57; goto _test_eof; 
	_test_eof58:  sm->cs = 58; goto _test_eof; 
	_test_eof59:  sm->cs = 59; goto _test_eof; 
	_test_eof60:  sm->cs = 60; goto _test_eof; 
	_test_eof61:  sm->cs = 61; goto _test_eof; 
	_test_eof62:  sm->cs = 62; goto _test_eof; 
	_test_eof63:  sm->cs = 63; goto _test_eof; 
	_test_eof64:  sm->cs = 64; goto _test_eof; 
	_test_eof65:  sm->cs = 65; goto _test_eof; 
	_test_eof66:  sm->cs = 66; goto _test_eof; 
	_test_eof67:  sm->cs = 67; goto _test_eof; 
	_test_eof68:  sm->cs = 68; goto _test_eof; 
	_test_eof69:  sm->cs = 69; goto _test_eof; 
	_test_eof70:  sm->cs = 70; goto _test_eof; 
	_test_eof71:  sm->cs = 71; goto _test_eof; 
	_test_eof72:  sm->cs = 72; goto _test_eof; 
	_test_eof73:  sm->cs = 73; goto _test_eof; 
	_test_eof74:  sm->cs = 74; goto _test_eof; 
	_test_eof75:  sm->cs = 75; goto _test_eof; 
	_test_eof76:  sm->cs = 76; goto _test_eof; 
	_test_eof77:  sm->cs = 77; goto _test_eof; 
	_test_eof78:  sm->cs = 78; goto _test_eof; 
	_test_eof79:  sm->cs = 79; goto _test_eof; 
	_test_eof80:  sm->cs = 80; goto _test_eof; 
	_test_eof81:  sm->cs = 81; goto _test_eof; 
	_test_eof82:  sm->cs = 82; goto _test_eof; 
	_test_eof83:  sm->cs = 83; goto _test_eof; 
	_test_eof84:  sm->cs = 84; goto _test_eof; 
	_test_eof85:  sm->cs = 85; goto _test_eof; 
	_test_eof86:  sm->cs = 86; goto _test_eof; 
	_test_eof87:  sm->cs = 87; goto _test_eof; 
	_test_eof88:  sm->cs = 88; goto _test_eof; 
	_test_eof89:  sm->cs = 89; goto _test_eof; 
	_test_eof90:  sm->cs = 90; goto _test_eof; 
	_test_eof91:  sm->cs = 91; goto _test_eof; 
	_test_eof92:  sm->cs = 92; goto _test_eof; 
	_test_eof93:  sm->cs = 93; goto _test_eof; 
	_test_eof94:  sm->cs = 94; goto _test_eof; 
	_test_eof95:  sm->cs = 95; goto _test_eof; 
	_test_eof96:  sm->cs = 96; goto _test_eof; 
	_test_eof97:  sm->cs = 97; goto _test_eof; 
	_test_eof98:  sm->cs = 98; goto _test_eof; 
	_test_eof99:  sm->cs = 99; goto _test_eof; 
	_test_eof100:  sm->cs = 100; goto _test_eof; 
	_test_eof101:  sm->cs = 101; goto _test_eof; 
	_test_eof102:  sm->cs = 102; goto _test_eof; 
	_test_eof103:  sm->cs = 103; goto _test_eof; 
	_test_eof104:  sm->cs = 104; goto _test_eof; 
	_test_eof105:  sm->cs = 105; goto _test_eof; 
	_test_eof106:  sm->cs = 106; goto _test_eof; 
	_test_eof107:  sm->cs = 107; goto _test_eof; 
	_test_eof108:  sm->cs = 108; goto _test_eof; 
	_test_eof109:  sm->cs = 109; goto _test_eof; 
	_test_eof110:  sm->cs = 110; goto _test_eof; 
	_test_eof111:  sm->cs = 111; goto _test_eof; 
	_test_eof112:  sm->cs = 112; goto _test_eof; 
	_test_eof113:  sm->cs = 113; goto _test_eof; 
	_test_eof114:  sm->cs = 114; goto _test_eof; 
	_test_eof115:  sm->cs = 115; goto _test_eof; 
	_test_eof116:  sm->cs = 116; goto _test_eof; 
	_test_eof117:  sm->cs = 117; goto _test_eof; 
	_test_eof118:  sm->cs = 118; goto _test_eof; 
	_test_eof119:  sm->cs = 119; goto _test_eof; 
	_test_eof120:  sm->cs = 120; goto _test_eof; 
	_test_eof121:  sm->cs = 121; goto _test_eof; 
	_test_eof122:  sm->cs = 122; goto _test_eof; 
	_test_eof123:  sm->cs = 123; goto _test_eof; 
	_test_eof124:  sm->cs = 124; goto _test_eof; 
	_test_eof125:  sm->cs = 125; goto _test_eof; 
	_test_eof126:  sm->cs = 126; goto _test_eof; 
	_test_eof127:  sm->cs = 127; goto _test_eof; 
	_test_eof128:  sm->cs = 128; goto _test_eof; 
	_test_eof129:  sm->cs = 129; goto _test_eof; 
	_test_eof130:  sm->cs = 130; goto _test_eof; 
	_test_eof131:  sm->cs = 131; goto _test_eof; 
	_test_eof132:  sm->cs = 132; goto _test_eof; 
	_test_eof723:  sm->cs = 723; goto _test_eof; 
	_test_eof133:  sm->cs = 133; goto _test_eof; 
	_test_eof134:  sm->cs = 134; goto _test_eof; 
	_test_eof135:  sm->cs = 135; goto _test_eof; 
	_test_eof136:  sm->cs = 136; goto _test_eof; 
	_test_eof137:  sm->cs = 137; goto _test_eof; 
	_test_eof138:  sm->cs = 138; goto _test_eof; 
	_test_eof139:  sm->cs = 139; goto _test_eof; 
	_test_eof140:  sm->cs = 140; goto _test_eof; 
	_test_eof724:  sm->cs = 724; goto _test_eof; 
	_test_eof141:  sm->cs = 141; goto _test_eof; 
	_test_eof142:  sm->cs = 142; goto _test_eof; 
	_test_eof143:  sm->cs = 143; goto _test_eof; 
	_test_eof144:  sm->cs = 144; goto _test_eof; 
	_test_eof145:  sm->cs = 145; goto _test_eof; 
	_test_eof146:  sm->cs = 146; goto _test_eof; 
	_test_eof147:  sm->cs = 147; goto _test_eof; 
	_test_eof725:  sm->cs = 725; goto _test_eof; 
	_test_eof148:  sm->cs = 148; goto _test_eof; 
	_test_eof149:  sm->cs = 149; goto _test_eof; 
	_test_eof150:  sm->cs = 150; goto _test_eof; 
	_test_eof151:  sm->cs = 151; goto _test_eof; 
	_test_eof152:  sm->cs = 152; goto _test_eof; 
	_test_eof726:  sm->cs = 726; goto _test_eof; 
	_test_eof727:  sm->cs = 727; goto _test_eof; 
	_test_eof728:  sm->cs = 728; goto _test_eof; 
	_test_eof153:  sm->cs = 153; goto _test_eof; 
	_test_eof154:  sm->cs = 154; goto _test_eof; 
	_test_eof155:  sm->cs = 155; goto _test_eof; 
	_test_eof156:  sm->cs = 156; goto _test_eof; 
	_test_eof157:  sm->cs = 157; goto _test_eof; 
	_test_eof158:  sm->cs = 158; goto _test_eof; 
	_test_eof159:  sm->cs = 159; goto _test_eof; 
	_test_eof160:  sm->cs = 160; goto _test_eof; 
	_test_eof161:  sm->cs = 161; goto _test_eof; 
	_test_eof162:  sm->cs = 162; goto _test_eof; 
	_test_eof163:  sm->cs = 163; goto _test_eof; 
	_test_eof164:  sm->cs = 164; goto _test_eof; 
	_test_eof165:  sm->cs = 165; goto _test_eof; 
	_test_eof166:  sm->cs = 166; goto _test_eof; 
	_test_eof167:  sm->cs = 167; goto _test_eof; 
	_test_eof729:  sm->cs = 729; goto _test_eof; 
	_test_eof730:  sm->cs = 730; goto _test_eof; 
	_test_eof731:  sm->cs = 731; goto _test_eof; 
	_test_eof168:  sm->cs = 168; goto _test_eof; 
	_test_eof169:  sm->cs = 169; goto _test_eof; 
	_test_eof170:  sm->cs = 170; goto _test_eof; 
	_test_eof171:  sm->cs = 171; goto _test_eof; 
	_test_eof172:  sm->cs = 172; goto _test_eof; 
	_test_eof173:  sm->cs = 173; goto _test_eof; 
	_test_eof174:  sm->cs = 174; goto _test_eof; 
	_test_eof175:  sm->cs = 175; goto _test_eof; 
	_test_eof176:  sm->cs = 176; goto _test_eof; 
	_test_eof177:  sm->cs = 177; goto _test_eof; 
	_test_eof178:  sm->cs = 178; goto _test_eof; 
	_test_eof179:  sm->cs = 179; goto _test_eof; 
	_test_eof180:  sm->cs = 180; goto _test_eof; 
	_test_eof181:  sm->cs = 181; goto _test_eof; 
	_test_eof182:  sm->cs = 182; goto _test_eof; 
	_test_eof732:  sm->cs = 732; goto _test_eof; 
	_test_eof733:  sm->cs = 733; goto _test_eof; 
	_test_eof183:  sm->cs = 183; goto _test_eof; 
	_test_eof184:  sm->cs = 184; goto _test_eof; 
	_test_eof734:  sm->cs = 734; goto _test_eof; 
	_test_eof185:  sm->cs = 185; goto _test_eof; 
	_test_eof186:  sm->cs = 186; goto _test_eof; 
	_test_eof187:  sm->cs = 187; goto _test_eof; 
	_test_eof188:  sm->cs = 188; goto _test_eof; 
	_test_eof189:  sm->cs = 189; goto _test_eof; 
	_test_eof190:  sm->cs = 190; goto _test_eof; 
	_test_eof191:  sm->cs = 191; goto _test_eof; 
	_test_eof735:  sm->cs = 735; goto _test_eof; 
	_test_eof192:  sm->cs = 192; goto _test_eof; 
	_test_eof193:  sm->cs = 193; goto _test_eof; 
	_test_eof194:  sm->cs = 194; goto _test_eof; 
	_test_eof195:  sm->cs = 195; goto _test_eof; 
	_test_eof196:  sm->cs = 196; goto _test_eof; 
	_test_eof197:  sm->cs = 197; goto _test_eof; 
	_test_eof198:  sm->cs = 198; goto _test_eof; 
	_test_eof736:  sm->cs = 736; goto _test_eof; 
	_test_eof737:  sm->cs = 737; goto _test_eof; 
	_test_eof738:  sm->cs = 738; goto _test_eof; 
	_test_eof199:  sm->cs = 199; goto _test_eof; 
	_test_eof200:  sm->cs = 200; goto _test_eof; 
	_test_eof201:  sm->cs = 201; goto _test_eof; 
	_test_eof202:  sm->cs = 202; goto _test_eof; 
	_test_eof739:  sm->cs = 739; goto _test_eof; 
	_test_eof203:  sm->cs = 203; goto _test_eof; 
	_test_eof204:  sm->cs = 204; goto _test_eof; 
	_test_eof205:  sm->cs = 205; goto _test_eof; 
	_test_eof206:  sm->cs = 206; goto _test_eof; 
	_test_eof207:  sm->cs = 207; goto _test_eof; 
	_test_eof208:  sm->cs = 208; goto _test_eof; 
	_test_eof209:  sm->cs = 209; goto _test_eof; 
	_test_eof210:  sm->cs = 210; goto _test_eof; 
	_test_eof211:  sm->cs = 211; goto _test_eof; 
	_test_eof212:  sm->cs = 212; goto _test_eof; 
	_test_eof213:  sm->cs = 213; goto _test_eof; 
	_test_eof214:  sm->cs = 214; goto _test_eof; 
	_test_eof215:  sm->cs = 215; goto _test_eof; 
	_test_eof216:  sm->cs = 216; goto _test_eof; 
	_test_eof217:  sm->cs = 217; goto _test_eof; 
	_test_eof218:  sm->cs = 218; goto _test_eof; 
	_test_eof219:  sm->cs = 219; goto _test_eof; 
	_test_eof740:  sm->cs = 740; goto _test_eof; 
	_test_eof220:  sm->cs = 220; goto _test_eof; 
	_test_eof221:  sm->cs = 221; goto _test_eof; 
	_test_eof222:  sm->cs = 222; goto _test_eof; 
	_test_eof223:  sm->cs = 223; goto _test_eof; 
	_test_eof224:  sm->cs = 224; goto _test_eof; 
	_test_eof225:  sm->cs = 225; goto _test_eof; 
	_test_eof226:  sm->cs = 226; goto _test_eof; 
	_test_eof227:  sm->cs = 227; goto _test_eof; 
	_test_eof228:  sm->cs = 228; goto _test_eof; 
	_test_eof741:  sm->cs = 741; goto _test_eof; 
	_test_eof229:  sm->cs = 229; goto _test_eof; 
	_test_eof230:  sm->cs = 230; goto _test_eof; 
	_test_eof231:  sm->cs = 231; goto _test_eof; 
	_test_eof232:  sm->cs = 232; goto _test_eof; 
	_test_eof233:  sm->cs = 233; goto _test_eof; 
	_test_eof234:  sm->cs = 234; goto _test_eof; 
	_test_eof742:  sm->cs = 742; goto _test_eof; 
	_test_eof235:  sm->cs = 235; goto _test_eof; 
	_test_eof236:  sm->cs = 236; goto _test_eof; 
	_test_eof237:  sm->cs = 237; goto _test_eof; 
	_test_eof238:  sm->cs = 238; goto _test_eof; 
	_test_eof239:  sm->cs = 239; goto _test_eof; 
	_test_eof240:  sm->cs = 240; goto _test_eof; 
	_test_eof241:  sm->cs = 241; goto _test_eof; 
	_test_eof743:  sm->cs = 743; goto _test_eof; 
	_test_eof744:  sm->cs = 744; goto _test_eof; 
	_test_eof242:  sm->cs = 242; goto _test_eof; 
	_test_eof243:  sm->cs = 243; goto _test_eof; 
	_test_eof244:  sm->cs = 244; goto _test_eof; 
	_test_eof245:  sm->cs = 245; goto _test_eof; 
	_test_eof745:  sm->cs = 745; goto _test_eof; 
	_test_eof246:  sm->cs = 246; goto _test_eof; 
	_test_eof247:  sm->cs = 247; goto _test_eof; 
	_test_eof248:  sm->cs = 248; goto _test_eof; 
	_test_eof249:  sm->cs = 249; goto _test_eof; 
	_test_eof250:  sm->cs = 250; goto _test_eof; 
	_test_eof746:  sm->cs = 746; goto _test_eof; 
	_test_eof251:  sm->cs = 251; goto _test_eof; 
	_test_eof252:  sm->cs = 252; goto _test_eof; 
	_test_eof253:  sm->cs = 253; goto _test_eof; 
	_test_eof254:  sm->cs = 254; goto _test_eof; 
	_test_eof747:  sm->cs = 747; goto _test_eof; 
	_test_eof748:  sm->cs = 748; goto _test_eof; 
	_test_eof255:  sm->cs = 255; goto _test_eof; 
	_test_eof256:  sm->cs = 256; goto _test_eof; 
	_test_eof257:  sm->cs = 257; goto _test_eof; 
	_test_eof258:  sm->cs = 258; goto _test_eof; 
	_test_eof259:  sm->cs = 259; goto _test_eof; 
	_test_eof260:  sm->cs = 260; goto _test_eof; 
	_test_eof261:  sm->cs = 261; goto _test_eof; 
	_test_eof262:  sm->cs = 262; goto _test_eof; 
	_test_eof749:  sm->cs = 749; goto _test_eof; 
	_test_eof750:  sm->cs = 750; goto _test_eof; 
	_test_eof263:  sm->cs = 263; goto _test_eof; 
	_test_eof264:  sm->cs = 264; goto _test_eof; 
	_test_eof265:  sm->cs = 265; goto _test_eof; 
	_test_eof266:  sm->cs = 266; goto _test_eof; 
	_test_eof267:  sm->cs = 267; goto _test_eof; 
	_test_eof751:  sm->cs = 751; goto _test_eof; 
	_test_eof268:  sm->cs = 268; goto _test_eof; 
	_test_eof269:  sm->cs = 269; goto _test_eof; 
	_test_eof270:  sm->cs = 270; goto _test_eof; 
	_test_eof271:  sm->cs = 271; goto _test_eof; 
	_test_eof272:  sm->cs = 272; goto _test_eof; 
	_test_eof273:  sm->cs = 273; goto _test_eof; 
	_test_eof752:  sm->cs = 752; goto _test_eof; 
	_test_eof753:  sm->cs = 753; goto _test_eof; 
	_test_eof274:  sm->cs = 274; goto _test_eof; 
	_test_eof275:  sm->cs = 275; goto _test_eof; 
	_test_eof276:  sm->cs = 276; goto _test_eof; 
	_test_eof277:  sm->cs = 277; goto _test_eof; 
	_test_eof278:  sm->cs = 278; goto _test_eof; 
	_test_eof279:  sm->cs = 279; goto _test_eof; 
	_test_eof754:  sm->cs = 754; goto _test_eof; 
	_test_eof280:  sm->cs = 280; goto _test_eof; 
	_test_eof755:  sm->cs = 755; goto _test_eof; 
	_test_eof281:  sm->cs = 281; goto _test_eof; 
	_test_eof282:  sm->cs = 282; goto _test_eof; 
	_test_eof283:  sm->cs = 283; goto _test_eof; 
	_test_eof284:  sm->cs = 284; goto _test_eof; 
	_test_eof285:  sm->cs = 285; goto _test_eof; 
	_test_eof286:  sm->cs = 286; goto _test_eof; 
	_test_eof287:  sm->cs = 287; goto _test_eof; 
	_test_eof288:  sm->cs = 288; goto _test_eof; 
	_test_eof289:  sm->cs = 289; goto _test_eof; 
	_test_eof290:  sm->cs = 290; goto _test_eof; 
	_test_eof291:  sm->cs = 291; goto _test_eof; 
	_test_eof292:  sm->cs = 292; goto _test_eof; 
	_test_eof756:  sm->cs = 756; goto _test_eof; 
	_test_eof757:  sm->cs = 757; goto _test_eof; 
	_test_eof293:  sm->cs = 293; goto _test_eof; 
	_test_eof294:  sm->cs = 294; goto _test_eof; 
	_test_eof295:  sm->cs = 295; goto _test_eof; 
	_test_eof296:  sm->cs = 296; goto _test_eof; 
	_test_eof297:  sm->cs = 297; goto _test_eof; 
	_test_eof298:  sm->cs = 298; goto _test_eof; 
	_test_eof299:  sm->cs = 299; goto _test_eof; 
	_test_eof300:  sm->cs = 300; goto _test_eof; 
	_test_eof301:  sm->cs = 301; goto _test_eof; 
	_test_eof302:  sm->cs = 302; goto _test_eof; 
	_test_eof303:  sm->cs = 303; goto _test_eof; 
	_test_eof758:  sm->cs = 758; goto _test_eof; 
	_test_eof759:  sm->cs = 759; goto _test_eof; 
	_test_eof304:  sm->cs = 304; goto _test_eof; 
	_test_eof305:  sm->cs = 305; goto _test_eof; 
	_test_eof306:  sm->cs = 306; goto _test_eof; 
	_test_eof307:  sm->cs = 307; goto _test_eof; 
	_test_eof308:  sm->cs = 308; goto _test_eof; 
	_test_eof760:  sm->cs = 760; goto _test_eof; 
	_test_eof761:  sm->cs = 761; goto _test_eof; 
	_test_eof309:  sm->cs = 309; goto _test_eof; 
	_test_eof310:  sm->cs = 310; goto _test_eof; 
	_test_eof311:  sm->cs = 311; goto _test_eof; 
	_test_eof312:  sm->cs = 312; goto _test_eof; 
	_test_eof313:  sm->cs = 313; goto _test_eof; 
	_test_eof762:  sm->cs = 762; goto _test_eof; 
	_test_eof314:  sm->cs = 314; goto _test_eof; 
	_test_eof315:  sm->cs = 315; goto _test_eof; 
	_test_eof316:  sm->cs = 316; goto _test_eof; 
	_test_eof317:  sm->cs = 317; goto _test_eof; 
	_test_eof763:  sm->cs = 763; goto _test_eof; 
	_test_eof318:  sm->cs = 318; goto _test_eof; 
	_test_eof319:  sm->cs = 319; goto _test_eof; 
	_test_eof320:  sm->cs = 320; goto _test_eof; 
	_test_eof321:  sm->cs = 321; goto _test_eof; 
	_test_eof322:  sm->cs = 322; goto _test_eof; 
	_test_eof323:  sm->cs = 323; goto _test_eof; 
	_test_eof324:  sm->cs = 324; goto _test_eof; 
	_test_eof325:  sm->cs = 325; goto _test_eof; 
	_test_eof326:  sm->cs = 326; goto _test_eof; 
	_test_eof764:  sm->cs = 764; goto _test_eof; 
	_test_eof765:  sm->cs = 765; goto _test_eof; 
	_test_eof327:  sm->cs = 327; goto _test_eof; 
	_test_eof328:  sm->cs = 328; goto _test_eof; 
	_test_eof329:  sm->cs = 329; goto _test_eof; 
	_test_eof330:  sm->cs = 330; goto _test_eof; 
	_test_eof331:  sm->cs = 331; goto _test_eof; 
	_test_eof332:  sm->cs = 332; goto _test_eof; 
	_test_eof333:  sm->cs = 333; goto _test_eof; 
	_test_eof766:  sm->cs = 766; goto _test_eof; 
	_test_eof767:  sm->cs = 767; goto _test_eof; 
	_test_eof334:  sm->cs = 334; goto _test_eof; 
	_test_eof335:  sm->cs = 335; goto _test_eof; 
	_test_eof336:  sm->cs = 336; goto _test_eof; 
	_test_eof337:  sm->cs = 337; goto _test_eof; 
	_test_eof768:  sm->cs = 768; goto _test_eof; 
	_test_eof769:  sm->cs = 769; goto _test_eof; 
	_test_eof338:  sm->cs = 338; goto _test_eof; 
	_test_eof339:  sm->cs = 339; goto _test_eof; 
	_test_eof340:  sm->cs = 340; goto _test_eof; 
	_test_eof341:  sm->cs = 341; goto _test_eof; 
	_test_eof342:  sm->cs = 342; goto _test_eof; 
	_test_eof343:  sm->cs = 343; goto _test_eof; 
	_test_eof344:  sm->cs = 344; goto _test_eof; 
	_test_eof345:  sm->cs = 345; goto _test_eof; 
	_test_eof346:  sm->cs = 346; goto _test_eof; 
	_test_eof347:  sm->cs = 347; goto _test_eof; 
	_test_eof770:  sm->cs = 770; goto _test_eof; 
	_test_eof348:  sm->cs = 348; goto _test_eof; 
	_test_eof349:  sm->cs = 349; goto _test_eof; 
	_test_eof350:  sm->cs = 350; goto _test_eof; 
	_test_eof351:  sm->cs = 351; goto _test_eof; 
	_test_eof352:  sm->cs = 352; goto _test_eof; 
	_test_eof353:  sm->cs = 353; goto _test_eof; 
	_test_eof354:  sm->cs = 354; goto _test_eof; 
	_test_eof355:  sm->cs = 355; goto _test_eof; 
	_test_eof356:  sm->cs = 356; goto _test_eof; 
	_test_eof357:  sm->cs = 357; goto _test_eof; 
	_test_eof358:  sm->cs = 358; goto _test_eof; 
	_test_eof359:  sm->cs = 359; goto _test_eof; 
	_test_eof360:  sm->cs = 360; goto _test_eof; 
	_test_eof361:  sm->cs = 361; goto _test_eof; 
	_test_eof771:  sm->cs = 771; goto _test_eof; 
	_test_eof362:  sm->cs = 362; goto _test_eof; 
	_test_eof363:  sm->cs = 363; goto _test_eof; 
	_test_eof364:  sm->cs = 364; goto _test_eof; 
	_test_eof365:  sm->cs = 365; goto _test_eof; 
	_test_eof366:  sm->cs = 366; goto _test_eof; 
	_test_eof367:  sm->cs = 367; goto _test_eof; 
	_test_eof368:  sm->cs = 368; goto _test_eof; 
	_test_eof772:  sm->cs = 772; goto _test_eof; 
	_test_eof369:  sm->cs = 369; goto _test_eof; 
	_test_eof370:  sm->cs = 370; goto _test_eof; 
	_test_eof371:  sm->cs = 371; goto _test_eof; 
	_test_eof372:  sm->cs = 372; goto _test_eof; 
	_test_eof373:  sm->cs = 373; goto _test_eof; 
	_test_eof374:  sm->cs = 374; goto _test_eof; 
	_test_eof773:  sm->cs = 773; goto _test_eof; 
	_test_eof774:  sm->cs = 774; goto _test_eof; 
	_test_eof375:  sm->cs = 375; goto _test_eof; 
	_test_eof376:  sm->cs = 376; goto _test_eof; 
	_test_eof377:  sm->cs = 377; goto _test_eof; 
	_test_eof378:  sm->cs = 378; goto _test_eof; 
	_test_eof379:  sm->cs = 379; goto _test_eof; 
	_test_eof775:  sm->cs = 775; goto _test_eof; 
	_test_eof776:  sm->cs = 776; goto _test_eof; 
	_test_eof380:  sm->cs = 380; goto _test_eof; 
	_test_eof381:  sm->cs = 381; goto _test_eof; 
	_test_eof382:  sm->cs = 382; goto _test_eof; 
	_test_eof383:  sm->cs = 383; goto _test_eof; 
	_test_eof384:  sm->cs = 384; goto _test_eof; 
	_test_eof777:  sm->cs = 777; goto _test_eof; 
	_test_eof778:  sm->cs = 778; goto _test_eof; 
	_test_eof385:  sm->cs = 385; goto _test_eof; 
	_test_eof386:  sm->cs = 386; goto _test_eof; 
	_test_eof387:  sm->cs = 387; goto _test_eof; 
	_test_eof388:  sm->cs = 388; goto _test_eof; 
	_test_eof389:  sm->cs = 389; goto _test_eof; 
	_test_eof390:  sm->cs = 390; goto _test_eof; 
	_test_eof391:  sm->cs = 391; goto _test_eof; 
	_test_eof392:  sm->cs = 392; goto _test_eof; 
	_test_eof779:  sm->cs = 779; goto _test_eof; 
	_test_eof393:  sm->cs = 393; goto _test_eof; 
	_test_eof394:  sm->cs = 394; goto _test_eof; 
	_test_eof395:  sm->cs = 395; goto _test_eof; 
	_test_eof396:  sm->cs = 396; goto _test_eof; 
	_test_eof397:  sm->cs = 397; goto _test_eof; 
	_test_eof398:  sm->cs = 398; goto _test_eof; 
	_test_eof399:  sm->cs = 399; goto _test_eof; 
	_test_eof400:  sm->cs = 400; goto _test_eof; 
	_test_eof401:  sm->cs = 401; goto _test_eof; 
	_test_eof402:  sm->cs = 402; goto _test_eof; 
	_test_eof403:  sm->cs = 403; goto _test_eof; 
	_test_eof404:  sm->cs = 404; goto _test_eof; 
	_test_eof405:  sm->cs = 405; goto _test_eof; 
	_test_eof780:  sm->cs = 780; goto _test_eof; 
	_test_eof406:  sm->cs = 406; goto _test_eof; 
	_test_eof407:  sm->cs = 407; goto _test_eof; 
	_test_eof408:  sm->cs = 408; goto _test_eof; 
	_test_eof409:  sm->cs = 409; goto _test_eof; 
	_test_eof410:  sm->cs = 410; goto _test_eof; 
	_test_eof411:  sm->cs = 411; goto _test_eof; 
	_test_eof412:  sm->cs = 412; goto _test_eof; 
	_test_eof413:  sm->cs = 413; goto _test_eof; 
	_test_eof414:  sm->cs = 414; goto _test_eof; 
	_test_eof415:  sm->cs = 415; goto _test_eof; 
	_test_eof416:  sm->cs = 416; goto _test_eof; 
	_test_eof417:  sm->cs = 417; goto _test_eof; 
	_test_eof418:  sm->cs = 418; goto _test_eof; 
	_test_eof419:  sm->cs = 419; goto _test_eof; 
	_test_eof420:  sm->cs = 420; goto _test_eof; 
	_test_eof421:  sm->cs = 421; goto _test_eof; 
	_test_eof422:  sm->cs = 422; goto _test_eof; 
	_test_eof423:  sm->cs = 423; goto _test_eof; 
	_test_eof424:  sm->cs = 424; goto _test_eof; 
	_test_eof425:  sm->cs = 425; goto _test_eof; 
	_test_eof426:  sm->cs = 426; goto _test_eof; 
	_test_eof427:  sm->cs = 427; goto _test_eof; 
	_test_eof428:  sm->cs = 428; goto _test_eof; 
	_test_eof429:  sm->cs = 429; goto _test_eof; 
	_test_eof430:  sm->cs = 430; goto _test_eof; 
	_test_eof431:  sm->cs = 431; goto _test_eof; 
	_test_eof432:  sm->cs = 432; goto _test_eof; 
	_test_eof433:  sm->cs = 433; goto _test_eof; 
	_test_eof434:  sm->cs = 434; goto _test_eof; 
	_test_eof435:  sm->cs = 435; goto _test_eof; 
	_test_eof436:  sm->cs = 436; goto _test_eof; 
	_test_eof437:  sm->cs = 437; goto _test_eof; 
	_test_eof438:  sm->cs = 438; goto _test_eof; 
	_test_eof439:  sm->cs = 439; goto _test_eof; 
	_test_eof440:  sm->cs = 440; goto _test_eof; 
	_test_eof441:  sm->cs = 441; goto _test_eof; 
	_test_eof442:  sm->cs = 442; goto _test_eof; 
	_test_eof443:  sm->cs = 443; goto _test_eof; 
	_test_eof444:  sm->cs = 444; goto _test_eof; 
	_test_eof445:  sm->cs = 445; goto _test_eof; 
	_test_eof446:  sm->cs = 446; goto _test_eof; 
	_test_eof447:  sm->cs = 447; goto _test_eof; 
	_test_eof448:  sm->cs = 448; goto _test_eof; 
	_test_eof449:  sm->cs = 449; goto _test_eof; 
	_test_eof450:  sm->cs = 450; goto _test_eof; 
	_test_eof451:  sm->cs = 451; goto _test_eof; 
	_test_eof452:  sm->cs = 452; goto _test_eof; 
	_test_eof453:  sm->cs = 453; goto _test_eof; 
	_test_eof454:  sm->cs = 454; goto _test_eof; 
	_test_eof455:  sm->cs = 455; goto _test_eof; 
	_test_eof456:  sm->cs = 456; goto _test_eof; 
	_test_eof457:  sm->cs = 457; goto _test_eof; 
	_test_eof458:  sm->cs = 458; goto _test_eof; 
	_test_eof459:  sm->cs = 459; goto _test_eof; 
	_test_eof460:  sm->cs = 460; goto _test_eof; 
	_test_eof461:  sm->cs = 461; goto _test_eof; 
	_test_eof462:  sm->cs = 462; goto _test_eof; 
	_test_eof463:  sm->cs = 463; goto _test_eof; 
	_test_eof464:  sm->cs = 464; goto _test_eof; 
	_test_eof465:  sm->cs = 465; goto _test_eof; 
	_test_eof466:  sm->cs = 466; goto _test_eof; 
	_test_eof467:  sm->cs = 467; goto _test_eof; 
	_test_eof468:  sm->cs = 468; goto _test_eof; 
	_test_eof469:  sm->cs = 469; goto _test_eof; 
	_test_eof470:  sm->cs = 470; goto _test_eof; 
	_test_eof471:  sm->cs = 471; goto _test_eof; 
	_test_eof472:  sm->cs = 472; goto _test_eof; 
	_test_eof473:  sm->cs = 473; goto _test_eof; 
	_test_eof474:  sm->cs = 474; goto _test_eof; 
	_test_eof475:  sm->cs = 475; goto _test_eof; 
	_test_eof476:  sm->cs = 476; goto _test_eof; 
	_test_eof477:  sm->cs = 477; goto _test_eof; 
	_test_eof478:  sm->cs = 478; goto _test_eof; 
	_test_eof479:  sm->cs = 479; goto _test_eof; 
	_test_eof480:  sm->cs = 480; goto _test_eof; 
	_test_eof481:  sm->cs = 481; goto _test_eof; 
	_test_eof482:  sm->cs = 482; goto _test_eof; 
	_test_eof483:  sm->cs = 483; goto _test_eof; 
	_test_eof484:  sm->cs = 484; goto _test_eof; 
	_test_eof485:  sm->cs = 485; goto _test_eof; 
	_test_eof486:  sm->cs = 486; goto _test_eof; 
	_test_eof487:  sm->cs = 487; goto _test_eof; 
	_test_eof488:  sm->cs = 488; goto _test_eof; 
	_test_eof489:  sm->cs = 489; goto _test_eof; 
	_test_eof490:  sm->cs = 490; goto _test_eof; 
	_test_eof491:  sm->cs = 491; goto _test_eof; 
	_test_eof492:  sm->cs = 492; goto _test_eof; 
	_test_eof493:  sm->cs = 493; goto _test_eof; 
	_test_eof494:  sm->cs = 494; goto _test_eof; 
	_test_eof495:  sm->cs = 495; goto _test_eof; 
	_test_eof496:  sm->cs = 496; goto _test_eof; 
	_test_eof497:  sm->cs = 497; goto _test_eof; 
	_test_eof498:  sm->cs = 498; goto _test_eof; 
	_test_eof499:  sm->cs = 499; goto _test_eof; 
	_test_eof500:  sm->cs = 500; goto _test_eof; 
	_test_eof501:  sm->cs = 501; goto _test_eof; 
	_test_eof502:  sm->cs = 502; goto _test_eof; 
	_test_eof503:  sm->cs = 503; goto _test_eof; 
	_test_eof504:  sm->cs = 504; goto _test_eof; 
	_test_eof505:  sm->cs = 505; goto _test_eof; 
	_test_eof506:  sm->cs = 506; goto _test_eof; 
	_test_eof507:  sm->cs = 507; goto _test_eof; 
	_test_eof508:  sm->cs = 508; goto _test_eof; 
	_test_eof509:  sm->cs = 509; goto _test_eof; 
	_test_eof510:  sm->cs = 510; goto _test_eof; 
	_test_eof511:  sm->cs = 511; goto _test_eof; 
	_test_eof512:  sm->cs = 512; goto _test_eof; 
	_test_eof513:  sm->cs = 513; goto _test_eof; 
	_test_eof514:  sm->cs = 514; goto _test_eof; 
	_test_eof515:  sm->cs = 515; goto _test_eof; 
	_test_eof516:  sm->cs = 516; goto _test_eof; 
	_test_eof517:  sm->cs = 517; goto _test_eof; 
	_test_eof518:  sm->cs = 518; goto _test_eof; 
	_test_eof519:  sm->cs = 519; goto _test_eof; 
	_test_eof520:  sm->cs = 520; goto _test_eof; 
	_test_eof521:  sm->cs = 521; goto _test_eof; 
	_test_eof522:  sm->cs = 522; goto _test_eof; 
	_test_eof523:  sm->cs = 523; goto _test_eof; 
	_test_eof524:  sm->cs = 524; goto _test_eof; 
	_test_eof525:  sm->cs = 525; goto _test_eof; 
	_test_eof526:  sm->cs = 526; goto _test_eof; 
	_test_eof527:  sm->cs = 527; goto _test_eof; 
	_test_eof528:  sm->cs = 528; goto _test_eof; 
	_test_eof529:  sm->cs = 529; goto _test_eof; 
	_test_eof530:  sm->cs = 530; goto _test_eof; 
	_test_eof531:  sm->cs = 531; goto _test_eof; 
	_test_eof532:  sm->cs = 532; goto _test_eof; 
	_test_eof533:  sm->cs = 533; goto _test_eof; 
	_test_eof534:  sm->cs = 534; goto _test_eof; 
	_test_eof535:  sm->cs = 535; goto _test_eof; 
	_test_eof536:  sm->cs = 536; goto _test_eof; 
	_test_eof537:  sm->cs = 537; goto _test_eof; 
	_test_eof538:  sm->cs = 538; goto _test_eof; 
	_test_eof539:  sm->cs = 539; goto _test_eof; 
	_test_eof540:  sm->cs = 540; goto _test_eof; 
	_test_eof541:  sm->cs = 541; goto _test_eof; 
	_test_eof542:  sm->cs = 542; goto _test_eof; 
	_test_eof543:  sm->cs = 543; goto _test_eof; 
	_test_eof544:  sm->cs = 544; goto _test_eof; 
	_test_eof545:  sm->cs = 545; goto _test_eof; 
	_test_eof546:  sm->cs = 546; goto _test_eof; 
	_test_eof547:  sm->cs = 547; goto _test_eof; 
	_test_eof548:  sm->cs = 548; goto _test_eof; 
	_test_eof549:  sm->cs = 549; goto _test_eof; 
	_test_eof550:  sm->cs = 550; goto _test_eof; 
	_test_eof551:  sm->cs = 551; goto _test_eof; 
	_test_eof552:  sm->cs = 552; goto _test_eof; 
	_test_eof553:  sm->cs = 553; goto _test_eof; 
	_test_eof554:  sm->cs = 554; goto _test_eof; 
	_test_eof555:  sm->cs = 555; goto _test_eof; 
	_test_eof556:  sm->cs = 556; goto _test_eof; 
	_test_eof557:  sm->cs = 557; goto _test_eof; 
	_test_eof558:  sm->cs = 558; goto _test_eof; 
	_test_eof559:  sm->cs = 559; goto _test_eof; 
	_test_eof560:  sm->cs = 560; goto _test_eof; 
	_test_eof561:  sm->cs = 561; goto _test_eof; 
	_test_eof562:  sm->cs = 562; goto _test_eof; 
	_test_eof563:  sm->cs = 563; goto _test_eof; 
	_test_eof564:  sm->cs = 564; goto _test_eof; 
	_test_eof565:  sm->cs = 565; goto _test_eof; 
	_test_eof566:  sm->cs = 566; goto _test_eof; 
	_test_eof567:  sm->cs = 567; goto _test_eof; 
	_test_eof568:  sm->cs = 568; goto _test_eof; 
	_test_eof569:  sm->cs = 569; goto _test_eof; 
	_test_eof570:  sm->cs = 570; goto _test_eof; 
	_test_eof571:  sm->cs = 571; goto _test_eof; 
	_test_eof572:  sm->cs = 572; goto _test_eof; 
	_test_eof573:  sm->cs = 573; goto _test_eof; 
	_test_eof574:  sm->cs = 574; goto _test_eof; 
	_test_eof575:  sm->cs = 575; goto _test_eof; 
	_test_eof576:  sm->cs = 576; goto _test_eof; 
	_test_eof577:  sm->cs = 577; goto _test_eof; 
	_test_eof578:  sm->cs = 578; goto _test_eof; 
	_test_eof579:  sm->cs = 579; goto _test_eof; 
	_test_eof580:  sm->cs = 580; goto _test_eof; 
	_test_eof581:  sm->cs = 581; goto _test_eof; 
	_test_eof582:  sm->cs = 582; goto _test_eof; 
	_test_eof583:  sm->cs = 583; goto _test_eof; 
	_test_eof584:  sm->cs = 584; goto _test_eof; 
	_test_eof585:  sm->cs = 585; goto _test_eof; 
	_test_eof586:  sm->cs = 586; goto _test_eof; 
	_test_eof587:  sm->cs = 587; goto _test_eof; 
	_test_eof588:  sm->cs = 588; goto _test_eof; 
	_test_eof589:  sm->cs = 589; goto _test_eof; 
	_test_eof590:  sm->cs = 590; goto _test_eof; 
	_test_eof591:  sm->cs = 591; goto _test_eof; 
	_test_eof592:  sm->cs = 592; goto _test_eof; 
	_test_eof593:  sm->cs = 593; goto _test_eof; 
	_test_eof594:  sm->cs = 594; goto _test_eof; 
	_test_eof595:  sm->cs = 595; goto _test_eof; 
	_test_eof596:  sm->cs = 596; goto _test_eof; 
	_test_eof597:  sm->cs = 597; goto _test_eof; 
	_test_eof598:  sm->cs = 598; goto _test_eof; 
	_test_eof599:  sm->cs = 599; goto _test_eof; 
	_test_eof600:  sm->cs = 600; goto _test_eof; 
	_test_eof601:  sm->cs = 601; goto _test_eof; 
	_test_eof602:  sm->cs = 602; goto _test_eof; 
	_test_eof603:  sm->cs = 603; goto _test_eof; 
	_test_eof604:  sm->cs = 604; goto _test_eof; 
	_test_eof605:  sm->cs = 605; goto _test_eof; 
	_test_eof606:  sm->cs = 606; goto _test_eof; 
	_test_eof607:  sm->cs = 607; goto _test_eof; 
	_test_eof608:  sm->cs = 608; goto _test_eof; 
	_test_eof609:  sm->cs = 609; goto _test_eof; 
	_test_eof610:  sm->cs = 610; goto _test_eof; 
	_test_eof611:  sm->cs = 611; goto _test_eof; 
	_test_eof612:  sm->cs = 612; goto _test_eof; 
	_test_eof613:  sm->cs = 613; goto _test_eof; 
	_test_eof614:  sm->cs = 614; goto _test_eof; 
	_test_eof615:  sm->cs = 615; goto _test_eof; 
	_test_eof616:  sm->cs = 616; goto _test_eof; 
	_test_eof617:  sm->cs = 617; goto _test_eof; 
	_test_eof618:  sm->cs = 618; goto _test_eof; 
	_test_eof619:  sm->cs = 619; goto _test_eof; 
	_test_eof620:  sm->cs = 620; goto _test_eof; 
	_test_eof621:  sm->cs = 621; goto _test_eof; 
	_test_eof622:  sm->cs = 622; goto _test_eof; 
	_test_eof623:  sm->cs = 623; goto _test_eof; 
	_test_eof624:  sm->cs = 624; goto _test_eof; 
	_test_eof625:  sm->cs = 625; goto _test_eof; 
	_test_eof626:  sm->cs = 626; goto _test_eof; 
	_test_eof627:  sm->cs = 627; goto _test_eof; 
	_test_eof628:  sm->cs = 628; goto _test_eof; 
	_test_eof629:  sm->cs = 629; goto _test_eof; 
	_test_eof630:  sm->cs = 630; goto _test_eof; 
	_test_eof631:  sm->cs = 631; goto _test_eof; 
	_test_eof632:  sm->cs = 632; goto _test_eof; 
	_test_eof633:  sm->cs = 633; goto _test_eof; 
	_test_eof634:  sm->cs = 634; goto _test_eof; 
	_test_eof635:  sm->cs = 635; goto _test_eof; 
	_test_eof636:  sm->cs = 636; goto _test_eof; 
	_test_eof637:  sm->cs = 637; goto _test_eof; 
	_test_eof638:  sm->cs = 638; goto _test_eof; 
	_test_eof639:  sm->cs = 639; goto _test_eof; 
	_test_eof640:  sm->cs = 640; goto _test_eof; 
	_test_eof641:  sm->cs = 641; goto _test_eof; 
	_test_eof642:  sm->cs = 642; goto _test_eof; 
	_test_eof643:  sm->cs = 643; goto _test_eof; 
	_test_eof644:  sm->cs = 644; goto _test_eof; 
	_test_eof645:  sm->cs = 645; goto _test_eof; 
	_test_eof646:  sm->cs = 646; goto _test_eof; 
	_test_eof647:  sm->cs = 647; goto _test_eof; 
	_test_eof648:  sm->cs = 648; goto _test_eof; 
	_test_eof649:  sm->cs = 649; goto _test_eof; 
	_test_eof650:  sm->cs = 650; goto _test_eof; 
	_test_eof651:  sm->cs = 651; goto _test_eof; 
	_test_eof652:  sm->cs = 652; goto _test_eof; 
	_test_eof653:  sm->cs = 653; goto _test_eof; 
	_test_eof654:  sm->cs = 654; goto _test_eof; 
	_test_eof655:  sm->cs = 655; goto _test_eof; 
	_test_eof656:  sm->cs = 656; goto _test_eof; 
	_test_eof657:  sm->cs = 657; goto _test_eof; 
	_test_eof658:  sm->cs = 658; goto _test_eof; 
	_test_eof659:  sm->cs = 659; goto _test_eof; 
	_test_eof660:  sm->cs = 660; goto _test_eof; 
	_test_eof661:  sm->cs = 661; goto _test_eof; 
	_test_eof662:  sm->cs = 662; goto _test_eof; 
	_test_eof663:  sm->cs = 663; goto _test_eof; 
	_test_eof664:  sm->cs = 664; goto _test_eof; 
	_test_eof665:  sm->cs = 665; goto _test_eof; 
	_test_eof666:  sm->cs = 666; goto _test_eof; 
	_test_eof667:  sm->cs = 667; goto _test_eof; 
	_test_eof668:  sm->cs = 668; goto _test_eof; 
	_test_eof669:  sm->cs = 669; goto _test_eof; 
	_test_eof670:  sm->cs = 670; goto _test_eof; 
	_test_eof671:  sm->cs = 671; goto _test_eof; 
	_test_eof672:  sm->cs = 672; goto _test_eof; 
	_test_eof673:  sm->cs = 673; goto _test_eof; 
	_test_eof674:  sm->cs = 674; goto _test_eof; 
	_test_eof781:  sm->cs = 781; goto _test_eof; 
	_test_eof782:  sm->cs = 782; goto _test_eof; 
	_test_eof675:  sm->cs = 675; goto _test_eof; 
	_test_eof676:  sm->cs = 676; goto _test_eof; 
	_test_eof677:  sm->cs = 677; goto _test_eof; 
	_test_eof678:  sm->cs = 678; goto _test_eof; 
	_test_eof679:  sm->cs = 679; goto _test_eof; 
	_test_eof680:  sm->cs = 680; goto _test_eof; 
	_test_eof681:  sm->cs = 681; goto _test_eof; 
	_test_eof783:  sm->cs = 783; goto _test_eof; 
	_test_eof784:  sm->cs = 784; goto _test_eof; 
	_test_eof785:  sm->cs = 785; goto _test_eof; 
	_test_eof786:  sm->cs = 786; goto _test_eof; 
	_test_eof682:  sm->cs = 682; goto _test_eof; 
	_test_eof683:  sm->cs = 683; goto _test_eof; 
	_test_eof684:  sm->cs = 684; goto _test_eof; 
	_test_eof685:  sm->cs = 685; goto _test_eof; 
	_test_eof686:  sm->cs = 686; goto _test_eof; 
	_test_eof787:  sm->cs = 787; goto _test_eof; 
	_test_eof788:  sm->cs = 788; goto _test_eof; 
	_test_eof687:  sm->cs = 687; goto _test_eof; 
	_test_eof688:  sm->cs = 688; goto _test_eof; 
	_test_eof689:  sm->cs = 689; goto _test_eof; 
	_test_eof690:  sm->cs = 690; goto _test_eof; 
	_test_eof691:  sm->cs = 691; goto _test_eof; 
	_test_eof692:  sm->cs = 692; goto _test_eof; 
	_test_eof693:  sm->cs = 693; goto _test_eof; 
	_test_eof694:  sm->cs = 694; goto _test_eof; 
	_test_eof695:  sm->cs = 695; goto _test_eof; 
	_test_eof696:  sm->cs = 696; goto _test_eof; 
	_test_eof697:  sm->cs = 697; goto _test_eof; 
	_test_eof698:  sm->cs = 698; goto _test_eof; 
	_test_eof699:  sm->cs = 699; goto _test_eof; 
	_test_eof700:  sm->cs = 700; goto _test_eof; 
	_test_eof701:  sm->cs = 701; goto _test_eof; 
	_test_eof702:  sm->cs = 702; goto _test_eof; 
	_test_eof703:  sm->cs = 703; goto _test_eof; 
	_test_eof704:  sm->cs = 704; goto _test_eof; 
	_test_eof705:  sm->cs = 705; goto _test_eof; 
	_test_eof706:  sm->cs = 706; goto _test_eof; 
	_test_eof707:  sm->cs = 707; goto _test_eof; 
	_test_eof708:  sm->cs = 708; goto _test_eof; 
	_test_eof709:  sm->cs = 709; goto _test_eof; 
	_test_eof710:  sm->cs = 710; goto _test_eof; 
	_test_eof711:  sm->cs = 711; goto _test_eof; 
	_test_eof712:  sm->cs = 712; goto _test_eof; 

	_test_eof: {}
	if ( ( sm->p) == ( sm->eof) )
	{
	switch (  sm->cs ) {
	case 714: goto tr0;
	case 0: goto tr0;
	case 715: goto tr793;
	case 716: goto tr793;
	case 1: goto tr2;
	case 717: goto tr794;
	case 718: goto tr794;
	case 2: goto tr2;
	case 719: goto tr793;
	case 3: goto tr2;
	case 720: goto tr797;
	case 721: goto tr793;
	case 4: goto tr2;
	case 5: goto tr2;
	case 6: goto tr2;
	case 7: goto tr2;
	case 8: goto tr2;
	case 9: goto tr2;
	case 10: goto tr2;
	case 11: goto tr2;
	case 12: goto tr2;
	case 13: goto tr2;
	case 14: goto tr2;
	case 15: goto tr2;
	case 16: goto tr2;
	case 722: goto tr804;
	case 17: goto tr2;
	case 18: goto tr2;
	case 19: goto tr2;
	case 20: goto tr2;
	case 21: goto tr2;
	case 22: goto tr2;
	case 23: goto tr2;
	case 24: goto tr2;
	case 25: goto tr2;
	case 26: goto tr2;
	case 27: goto tr2;
	case 28: goto tr2;
	case 29: goto tr2;
	case 30: goto tr2;
	case 31: goto tr2;
	case 32: goto tr2;
	case 33: goto tr2;
	case 34: goto tr2;
	case 35: goto tr2;
	case 36: goto tr2;
	case 37: goto tr2;
	case 38: goto tr2;
	case 39: goto tr2;
	case 40: goto tr2;
	case 41: goto tr2;
	case 42: goto tr2;
	case 43: goto tr2;
	case 44: goto tr2;
	case 45: goto tr2;
	case 46: goto tr2;
	case 47: goto tr2;
	case 48: goto tr2;
	case 49: goto tr2;
	case 50: goto tr2;
	case 51: goto tr2;
	case 52: goto tr2;
	case 53: goto tr2;
	case 54: goto tr2;
	case 55: goto tr2;
	case 56: goto tr2;
	case 57: goto tr2;
	case 58: goto tr2;
	case 59: goto tr2;
	case 60: goto tr2;
	case 61: goto tr2;
	case 62: goto tr2;
	case 63: goto tr2;
	case 64: goto tr2;
	case 65: goto tr2;
	case 66: goto tr2;
	case 67: goto tr2;
	case 68: goto tr2;
	case 69: goto tr2;
	case 70: goto tr2;
	case 71: goto tr2;
	case 72: goto tr2;
	case 73: goto tr2;
	case 74: goto tr2;
	case 75: goto tr2;
	case 76: goto tr2;
	case 77: goto tr2;
	case 78: goto tr2;
	case 79: goto tr2;
	case 80: goto tr2;
	case 81: goto tr2;
	case 82: goto tr2;
	case 83: goto tr2;
	case 84: goto tr2;
	case 85: goto tr2;
	case 86: goto tr2;
	case 87: goto tr2;
	case 88: goto tr2;
	case 89: goto tr2;
	case 90: goto tr2;
	case 91: goto tr2;
	case 92: goto tr2;
	case 93: goto tr2;
	case 94: goto tr2;
	case 95: goto tr2;
	case 96: goto tr2;
	case 97: goto tr2;
	case 98: goto tr2;
	case 99: goto tr2;
	case 100: goto tr2;
	case 101: goto tr2;
	case 102: goto tr2;
	case 103: goto tr2;
	case 104: goto tr2;
	case 105: goto tr2;
	case 106: goto tr2;
	case 107: goto tr2;
	case 108: goto tr2;
	case 109: goto tr2;
	case 110: goto tr2;
	case 111: goto tr2;
	case 112: goto tr2;
	case 113: goto tr2;
	case 114: goto tr2;
	case 115: goto tr2;
	case 116: goto tr2;
	case 117: goto tr2;
	case 118: goto tr2;
	case 119: goto tr2;
	case 120: goto tr2;
	case 121: goto tr2;
	case 122: goto tr2;
	case 123: goto tr2;
	case 124: goto tr2;
	case 125: goto tr2;
	case 126: goto tr2;
	case 127: goto tr2;
	case 128: goto tr2;
	case 129: goto tr2;
	case 130: goto tr2;
	case 131: goto tr2;
	case 132: goto tr2;
	case 723: goto tr805;
	case 133: goto tr2;
	case 134: goto tr2;
	case 135: goto tr2;
	case 136: goto tr2;
	case 137: goto tr2;
	case 138: goto tr2;
	case 139: goto tr2;
	case 140: goto tr2;
	case 724: goto tr806;
	case 141: goto tr2;
	case 142: goto tr2;
	case 143: goto tr2;
	case 144: goto tr2;
	case 145: goto tr2;
	case 146: goto tr2;
	case 147: goto tr2;
	case 725: goto tr807;
	case 148: goto tr2;
	case 149: goto tr2;
	case 150: goto tr2;
	case 151: goto tr2;
	case 152: goto tr2;
	case 726: goto tr793;
	case 728: goto tr811;
	case 153: goto tr163;
	case 154: goto tr163;
	case 155: goto tr163;
	case 156: goto tr163;
	case 157: goto tr163;
	case 158: goto tr163;
	case 159: goto tr163;
	case 160: goto tr163;
	case 161: goto tr163;
	case 162: goto tr163;
	case 163: goto tr163;
	case 164: goto tr163;
	case 165: goto tr163;
	case 166: goto tr163;
	case 167: goto tr163;
	case 730: goto tr840;
	case 731: goto tr845;
	case 168: goto tr186;
	case 169: goto tr188;
	case 170: goto tr188;
	case 171: goto tr188;
	case 172: goto tr186;
	case 173: goto tr186;
	case 174: goto tr186;
	case 175: goto tr186;
	case 176: goto tr186;
	case 177: goto tr186;
	case 178: goto tr186;
	case 179: goto tr186;
	case 180: goto tr186;
	case 181: goto tr202;
	case 182: goto tr202;
	case 732: goto tr847;
	case 733: goto tr847;
	case 183: goto tr202;
	case 184: goto tr202;
	case 734: goto tr849;
	case 185: goto tr202;
	case 186: goto tr202;
	case 187: goto tr186;
	case 188: goto tr186;
	case 189: goto tr186;
	case 190: goto tr186;
	case 191: goto tr186;
	case 735: goto tr851;
	case 192: goto tr202;
	case 193: goto tr186;
	case 194: goto tr186;
	case 195: goto tr186;
	case 196: goto tr186;
	case 197: goto tr186;
	case 198: goto tr186;
	case 736: goto tr852;
	case 737: goto tr853;
	case 738: goto tr854;
	case 199: goto tr224;
	case 200: goto tr224;
	case 201: goto tr224;
	case 202: goto tr224;
	case 739: goto tr856;
	case 203: goto tr224;
	case 204: goto tr224;
	case 205: goto tr224;
	case 206: goto tr224;
	case 207: goto tr224;
	case 208: goto tr224;
	case 209: goto tr224;
	case 210: goto tr224;
	case 211: goto tr224;
	case 212: goto tr224;
	case 213: goto tr224;
	case 214: goto tr224;
	case 215: goto tr224;
	case 216: goto tr224;
	case 217: goto tr224;
	case 218: goto tr224;
	case 219: goto tr224;
	case 740: goto tr854;
	case 220: goto tr224;
	case 221: goto tr224;
	case 222: goto tr224;
	case 223: goto tr224;
	case 224: goto tr224;
	case 225: goto tr224;
	case 226: goto tr224;
	case 227: goto tr224;
	case 228: goto tr224;
	case 741: goto tr854;
	case 229: goto tr224;
	case 230: goto tr224;
	case 231: goto tr224;
	case 232: goto tr224;
	case 233: goto tr224;
	case 234: goto tr224;
	case 742: goto tr860;
	case 235: goto tr224;
	case 236: goto tr224;
	case 237: goto tr224;
	case 238: goto tr224;
	case 239: goto tr224;
	case 240: goto tr224;
	case 241: goto tr224;
	case 743: goto tr862;
	case 744: goto tr854;
	case 242: goto tr224;
	case 243: goto tr224;
	case 244: goto tr224;
	case 245: goto tr224;
	case 745: goto tr867;
	case 246: goto tr224;
	case 247: goto tr224;
	case 248: goto tr224;
	case 249: goto tr224;
	case 250: goto tr224;
	case 746: goto tr869;
	case 251: goto tr224;
	case 252: goto tr224;
	case 253: goto tr224;
	case 254: goto tr224;
	case 747: goto tr871;
	case 748: goto tr854;
	case 255: goto tr224;
	case 256: goto tr224;
	case 257: goto tr224;
	case 258: goto tr224;
	case 259: goto tr224;
	case 260: goto tr224;
	case 261: goto tr224;
	case 262: goto tr224;
	case 749: goto tr874;
	case 750: goto tr854;
	case 263: goto tr224;
	case 264: goto tr224;
	case 265: goto tr224;
	case 266: goto tr224;
	case 267: goto tr224;
	case 751: goto tr878;
	case 268: goto tr224;
	case 269: goto tr224;
	case 270: goto tr224;
	case 271: goto tr224;
	case 272: goto tr224;
	case 273: goto tr224;
	case 752: goto tr880;
	case 753: goto tr854;
	case 274: goto tr224;
	case 275: goto tr224;
	case 276: goto tr224;
	case 277: goto tr224;
	case 278: goto tr224;
	case 279: goto tr224;
	case 754: goto tr883;
	case 280: goto tr224;
	case 755: goto tr854;
	case 281: goto tr224;
	case 282: goto tr224;
	case 283: goto tr224;
	case 284: goto tr224;
	case 285: goto tr224;
	case 286: goto tr224;
	case 287: goto tr224;
	case 288: goto tr224;
	case 289: goto tr224;
	case 290: goto tr224;
	case 291: goto tr224;
	case 292: goto tr224;
	case 756: goto tr885;
	case 757: goto tr854;
	case 293: goto tr224;
	case 294: goto tr224;
	case 295: goto tr224;
	case 296: goto tr224;
	case 297: goto tr224;
	case 298: goto tr224;
	case 299: goto tr224;
	case 300: goto tr224;
	case 301: goto tr224;
	case 302: goto tr224;
	case 303: goto tr224;
	case 758: goto tr888;
	case 759: goto tr854;
	case 304: goto tr224;
	case 305: goto tr224;
	case 306: goto tr224;
	case 307: goto tr224;
	case 308: goto tr224;
	case 760: goto tr891;
	case 761: goto tr854;
	case 309: goto tr224;
	case 310: goto tr224;
	case 311: goto tr224;
	case 312: goto tr224;
	case 313: goto tr224;
	case 762: goto tr894;
	case 314: goto tr224;
	case 315: goto tr224;
	case 316: goto tr224;
	case 317: goto tr224;
	case 763: goto tr896;
	case 318: goto tr224;
	case 319: goto tr224;
	case 320: goto tr224;
	case 321: goto tr224;
	case 322: goto tr224;
	case 323: goto tr224;
	case 324: goto tr224;
	case 325: goto tr224;
	case 326: goto tr224;
	case 764: goto tr898;
	case 765: goto tr854;
	case 327: goto tr224;
	case 328: goto tr224;
	case 329: goto tr224;
	case 330: goto tr224;
	case 331: goto tr224;
	case 332: goto tr224;
	case 333: goto tr224;
	case 766: goto tr901;
	case 767: goto tr854;
	case 334: goto tr224;
	case 335: goto tr224;
	case 336: goto tr224;
	case 337: goto tr224;
	case 768: goto tr904;
	case 769: goto tr854;
	case 338: goto tr224;
	case 339: goto tr224;
	case 340: goto tr224;
	case 341: goto tr224;
	case 342: goto tr224;
	case 343: goto tr224;
	case 344: goto tr224;
	case 345: goto tr224;
	case 346: goto tr224;
	case 347: goto tr224;
	case 770: goto tr910;
	case 348: goto tr224;
	case 349: goto tr224;
	case 350: goto tr224;
	case 351: goto tr224;
	case 352: goto tr224;
	case 353: goto tr224;
	case 354: goto tr224;
	case 355: goto tr224;
	case 356: goto tr224;
	case 357: goto tr224;
	case 358: goto tr224;
	case 359: goto tr224;
	case 360: goto tr224;
	case 361: goto tr224;
	case 771: goto tr912;
	case 362: goto tr224;
	case 363: goto tr224;
	case 364: goto tr224;
	case 365: goto tr224;
	case 366: goto tr224;
	case 367: goto tr224;
	case 368: goto tr224;
	case 772: goto tr914;
	case 369: goto tr224;
	case 370: goto tr224;
	case 371: goto tr224;
	case 372: goto tr224;
	case 373: goto tr224;
	case 374: goto tr224;
	case 773: goto tr916;
	case 774: goto tr854;
	case 375: goto tr224;
	case 376: goto tr224;
	case 377: goto tr224;
	case 378: goto tr224;
	case 379: goto tr224;
	case 775: goto tr919;
	case 776: goto tr854;
	case 380: goto tr224;
	case 381: goto tr224;
	case 382: goto tr224;
	case 383: goto tr224;
	case 384: goto tr224;
	case 777: goto tr922;
	case 778: goto tr854;
	case 385: goto tr224;
	case 386: goto tr224;
	case 387: goto tr224;
	case 388: goto tr224;
	case 389: goto tr224;
	case 390: goto tr224;
	case 391: goto tr224;
	case 392: goto tr224;
	case 779: goto tr934;
	case 393: goto tr224;
	case 394: goto tr224;
	case 395: goto tr224;
	case 396: goto tr224;
	case 397: goto tr224;
	case 398: goto tr224;
	case 399: goto tr224;
	case 400: goto tr224;
	case 401: goto tr224;
	case 402: goto tr224;
	case 403: goto tr224;
	case 404: goto tr224;
	case 405: goto tr224;
	case 780: goto tr935;
	case 406: goto tr224;
	case 407: goto tr224;
	case 408: goto tr224;
	case 409: goto tr224;
	case 410: goto tr224;
	case 411: goto tr224;
	case 412: goto tr224;
	case 413: goto tr224;
	case 414: goto tr224;
	case 415: goto tr224;
	case 416: goto tr224;
	case 417: goto tr224;
	case 418: goto tr224;
	case 419: goto tr224;
	case 420: goto tr224;
	case 421: goto tr224;
	case 422: goto tr224;
	case 423: goto tr224;
	case 424: goto tr224;
	case 425: goto tr224;
	case 426: goto tr224;
	case 427: goto tr224;
	case 428: goto tr224;
	case 429: goto tr224;
	case 430: goto tr224;
	case 431: goto tr224;
	case 432: goto tr224;
	case 433: goto tr224;
	case 434: goto tr224;
	case 435: goto tr224;
	case 436: goto tr224;
	case 437: goto tr224;
	case 438: goto tr224;
	case 439: goto tr224;
	case 440: goto tr224;
	case 441: goto tr224;
	case 442: goto tr224;
	case 443: goto tr224;
	case 444: goto tr224;
	case 445: goto tr224;
	case 446: goto tr224;
	case 447: goto tr224;
	case 448: goto tr224;
	case 449: goto tr224;
	case 450: goto tr224;
	case 451: goto tr224;
	case 452: goto tr224;
	case 453: goto tr224;
	case 454: goto tr224;
	case 455: goto tr224;
	case 456: goto tr224;
	case 457: goto tr224;
	case 458: goto tr224;
	case 459: goto tr224;
	case 460: goto tr224;
	case 461: goto tr224;
	case 462: goto tr224;
	case 463: goto tr224;
	case 464: goto tr224;
	case 465: goto tr224;
	case 466: goto tr224;
	case 467: goto tr224;
	case 468: goto tr224;
	case 469: goto tr224;
	case 470: goto tr224;
	case 471: goto tr224;
	case 472: goto tr224;
	case 473: goto tr224;
	case 474: goto tr224;
	case 475: goto tr224;
	case 476: goto tr224;
	case 477: goto tr224;
	case 478: goto tr224;
	case 479: goto tr224;
	case 480: goto tr224;
	case 481: goto tr224;
	case 482: goto tr224;
	case 483: goto tr224;
	case 484: goto tr224;
	case 485: goto tr224;
	case 486: goto tr224;
	case 487: goto tr224;
	case 488: goto tr224;
	case 489: goto tr224;
	case 490: goto tr224;
	case 491: goto tr224;
	case 492: goto tr224;
	case 493: goto tr224;
	case 494: goto tr224;
	case 495: goto tr224;
	case 496: goto tr224;
	case 497: goto tr224;
	case 498: goto tr224;
	case 499: goto tr224;
	case 500: goto tr224;
	case 501: goto tr224;
	case 502: goto tr224;
	case 503: goto tr224;
	case 504: goto tr224;
	case 505: goto tr224;
	case 506: goto tr224;
	case 507: goto tr224;
	case 508: goto tr224;
	case 509: goto tr224;
	case 510: goto tr224;
	case 511: goto tr224;
	case 512: goto tr224;
	case 513: goto tr224;
	case 514: goto tr224;
	case 515: goto tr224;
	case 516: goto tr224;
	case 517: goto tr224;
	case 518: goto tr224;
	case 519: goto tr224;
	case 520: goto tr224;
	case 521: goto tr224;
	case 522: goto tr224;
	case 523: goto tr224;
	case 524: goto tr224;
	case 525: goto tr224;
	case 526: goto tr224;
	case 527: goto tr224;
	case 528: goto tr224;
	case 529: goto tr224;
	case 530: goto tr224;
	case 531: goto tr224;
	case 532: goto tr224;
	case 533: goto tr224;
	case 534: goto tr224;
	case 535: goto tr224;
	case 536: goto tr224;
	case 537: goto tr224;
	case 538: goto tr224;
	case 539: goto tr224;
	case 540: goto tr224;
	case 541: goto tr224;
	case 542: goto tr224;
	case 543: goto tr224;
	case 544: goto tr224;
	case 545: goto tr224;
	case 546: goto tr224;
	case 547: goto tr224;
	case 548: goto tr224;
	case 549: goto tr224;
	case 550: goto tr224;
	case 551: goto tr224;
	case 552: goto tr224;
	case 553: goto tr224;
	case 554: goto tr224;
	case 555: goto tr224;
	case 556: goto tr224;
	case 557: goto tr224;
	case 558: goto tr224;
	case 559: goto tr224;
	case 560: goto tr224;
	case 561: goto tr224;
	case 562: goto tr224;
	case 563: goto tr224;
	case 564: goto tr224;
	case 565: goto tr224;
	case 566: goto tr224;
	case 567: goto tr224;
	case 568: goto tr224;
	case 569: goto tr224;
	case 570: goto tr224;
	case 571: goto tr224;
	case 572: goto tr224;
	case 573: goto tr224;
	case 574: goto tr224;
	case 575: goto tr224;
	case 576: goto tr224;
	case 577: goto tr224;
	case 578: goto tr224;
	case 579: goto tr224;
	case 580: goto tr224;
	case 581: goto tr224;
	case 582: goto tr224;
	case 583: goto tr224;
	case 584: goto tr224;
	case 585: goto tr224;
	case 586: goto tr224;
	case 587: goto tr224;
	case 588: goto tr224;
	case 589: goto tr224;
	case 590: goto tr224;
	case 591: goto tr224;
	case 592: goto tr224;
	case 593: goto tr224;
	case 594: goto tr224;
	case 595: goto tr224;
	case 596: goto tr224;
	case 597: goto tr224;
	case 598: goto tr224;
	case 599: goto tr224;
	case 600: goto tr224;
	case 601: goto tr224;
	case 602: goto tr224;
	case 603: goto tr224;
	case 604: goto tr224;
	case 605: goto tr224;
	case 606: goto tr224;
	case 607: goto tr224;
	case 608: goto tr224;
	case 609: goto tr224;
	case 610: goto tr224;
	case 611: goto tr224;
	case 612: goto tr224;
	case 613: goto tr224;
	case 614: goto tr224;
	case 615: goto tr224;
	case 616: goto tr224;
	case 617: goto tr224;
	case 618: goto tr224;
	case 619: goto tr224;
	case 620: goto tr224;
	case 621: goto tr224;
	case 622: goto tr224;
	case 623: goto tr224;
	case 624: goto tr224;
	case 625: goto tr224;
	case 626: goto tr224;
	case 627: goto tr224;
	case 628: goto tr224;
	case 629: goto tr224;
	case 630: goto tr224;
	case 631: goto tr224;
	case 632: goto tr224;
	case 633: goto tr224;
	case 634: goto tr224;
	case 635: goto tr224;
	case 636: goto tr224;
	case 637: goto tr224;
	case 638: goto tr224;
	case 639: goto tr224;
	case 640: goto tr224;
	case 641: goto tr224;
	case 642: goto tr224;
	case 643: goto tr224;
	case 644: goto tr224;
	case 645: goto tr224;
	case 646: goto tr224;
	case 647: goto tr224;
	case 648: goto tr224;
	case 649: goto tr224;
	case 650: goto tr224;
	case 651: goto tr224;
	case 652: goto tr224;
	case 653: goto tr224;
	case 654: goto tr224;
	case 655: goto tr224;
	case 656: goto tr224;
	case 657: goto tr224;
	case 658: goto tr224;
	case 659: goto tr224;
	case 660: goto tr224;
	case 661: goto tr224;
	case 662: goto tr224;
	case 663: goto tr224;
	case 664: goto tr224;
	case 665: goto tr224;
	case 666: goto tr224;
	case 667: goto tr224;
	case 668: goto tr224;
	case 669: goto tr224;
	case 670: goto tr224;
	case 671: goto tr224;
	case 672: goto tr224;
	case 673: goto tr224;
	case 674: goto tr224;
	case 781: goto tr854;
	case 782: goto tr854;
	case 675: goto tr224;
	case 676: goto tr224;
	case 677: goto tr224;
	case 678: goto tr224;
	case 679: goto tr224;
	case 680: goto tr224;
	case 681: goto tr224;
	case 784: goto tr941;
	case 786: goto tr945;
	case 682: goto tr744;
	case 683: goto tr744;
	case 684: goto tr744;
	case 685: goto tr744;
	case 686: goto tr744;
	case 788: goto tr949;
	case 687: goto tr750;
	case 688: goto tr750;
	case 689: goto tr750;
	case 690: goto tr750;
	case 691: goto tr750;
	case 692: goto tr750;
	case 693: goto tr750;
	case 694: goto tr750;
	case 695: goto tr750;
	case 696: goto tr750;
	case 697: goto tr750;
	case 698: goto tr750;
	case 699: goto tr750;
	case 700: goto tr750;
	case 701: goto tr750;
	case 702: goto tr750;
	case 703: goto tr750;
	case 704: goto tr750;
	case 705: goto tr750;
	case 706: goto tr750;
	case 707: goto tr750;
	case 708: goto tr750;
	case 709: goto tr750;
	case 710: goto tr750;
	case 711: goto tr750;
	case 712: goto tr750;
	}
	}

	}

#line 1156 "ext/dtext/dtext.cpp.rl"

  sm->dstack_close_all();

  return DTextResult { sm->output, sm->posts };
}

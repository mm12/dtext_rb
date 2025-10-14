require 'minitest/autorun'
require 'dtext'

class DTextTest < Minitest::Test
  def assert_parse_id_link(class_name, url, input, display: input)
    assert_parse(%{<p><a class="dtext-link dtext-id-link #{class_name}" href="#{url}">#{display}</a></p>}, input)
  end

  def assert_parse(expected, input, **options)

    if expected.nil?
      assert_nil(str_part = DText.parse(input, **options))
    else
      str_part = DText.parse(input, **options)[0]
      assert_equal(expected, str_part)
    end
  end

  def assert_color(prefix, color)
    str_part = DText.parse("[color=#{color}]test[/color]", allow_color: true)[0]
    assert_equal("#{prefix}#{color}\">test</span></p>", str_part)
  end

  def test_legacy_table
    assert_parse("<table class=\"striped\"><thead><tr><th>test1 \\| test2 </th><th> test2</th></tr></thead><tbody><tr><td>abc </td><td> 123</td></tr></tbody></table>", <<~END
[ltable]
test1 \\| test2 | test2
abc | 123
[/ltable]
    END
    )
    assert_parse("<table class=\"striped\"><thead><tr><th>test1 </th><th> test2</th></tr></thead><tbody><tr><td>abc </td><td> 123</td></tr></tbody></table><table class=\"striped\"><thead><tr><th>test1 </th><th> test2</th></tr></thead><tbody><tr><td>abc </td><td> 123</td></tr></tbody></table>", <<~END
[ltable]
test1 | test2
abc | 123
[/ltable]
[ltable]
test1 | test2
abc | 123
[/ltable]
    END
    )
    assert_parse("<table class=\"striped\"><thead><tr><th>test1</th></tr></thead><tbody></tbody></table>", <<~END
[ltable]
test1
[/ltable]
    END
    )
    assert_parse("<table class=\"striped\"><thead><tr><th>test1</th></tr></thead><tbody><tr><td>test2</td></tr></tbody></table>", <<~END
[ltable]test1
test2[/ltable]
    END
    )
  end

  def test_relative_urls
    assert_parse('<p><a class="dtext-link dtext-id-link dtext-post-id-link" href="http://danbooru.donmai.us/posts/1234">post #1234</a></p>', "post #1234", base_url: "http://danbooru.donmai.us")
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="http://danbooru.donmai.us/posts">posts</a></p>', '"posts":/posts', base_url: "http://danbooru.donmai.us")
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="https://example.com/posts">posts</a></p>', '"posts":https://example.com/posts', base_url: "https://e621.net")
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-wiki-link" href="https://e621.net/wiki_pages/show_or_new?title=abc">abc</a></p>', '[[abc]]', base_url: "https://e621.net")
  end

  def test_thumbnails
    parsed = DText.parse("thumb #123")
    assert_equal(parsed[1], [123])
    assert_equal(parsed[0], "<p><a class=\"dtext-link dtext-id-link dtext-post-id-link thumb-placeholder-link\" data-id=\"123\" href=\"/posts/123\">post #123</a></p>")
    parsed = DText.parse("thumb #123 "*10, max_thumbs: 5)
    assert_equal(parsed[1], [123]*5)
  end

  def test_thumbails_max_count
    assert_parse('<p><a class="dtext-link dtext-id-link dtext-post-id-link" href="/posts/1">post #1</a></p>', "thumb #1", max_thumbs: 0);
  end

  def test_args
    assert_parse(nil, nil)
    assert_parse("", "")
    assert_raises(TypeError) { DText.parse(42) }
  end

  def test_sanitize_heart
    assert_parse('<p>&lt;3</p>', "<3")
  end

  def test_sanitize_less_than
    assert_parse('<p>&lt;</p>', "<")
  end

  def test_sanitize_greater_than
    assert_parse('<p>&gt;</p>', ">")
  end

  def test_sanitize_ampersand
    assert_parse('<p>&amp;</p>', "&")
  end

  def test_wiki_links
    assert_parse("<p>a <a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=b\">b</a> c</p>", "a [[b]] c")
  end

  def test_wiki_links_spoiler
    assert_parse("<p>a <a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=spoiler\">spoiler</a> c</p>", "a [[spoiler]] c")
  end

  def test_wiki_links_aliased
    assert_parse("<p><a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=wiki_page\">Some Text</a></p>", "[[wiki page|Some Text]]")
  end

  def test_wiki_links_utf8
    assert_parse("<p><a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=pok%C3%A9mon\">pokémon</a></p>", "[[pokémon]]")
  end

  def test_wiki_links_lowercase_utf8
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-wiki-link" href="/wiki_pages/show_or_new?title=%C5%8Ckami">ŌkamI</a></p>', "[[ŌkamI]]")
  end

  def test_wiki_links_edge
    assert_parse("<p>[[|_|]]</p>", "[[|_|]]")
    assert_parse("<p>[[||_||]]</p>", "[[||_||]]")
  end

  def test_wiki_links_nested_b
    assert_parse("<p><strong>[[</strong>tag<strong>]]</strong></p>", "[b][[[/b]tag[b]]][/b]")
  end

  def test_spoilers
    assert_parse("<p>this is <span class=\"spoiler\">an inline spoiler</span>.</p>", "this is [spoiler]an inline spoiler[/spoiler].")
    assert_parse("<p>this is <span class=\"spoiler\">an inline spoiler</span>.</p>", "this is [SPOILERS]an inline spoiler[/SPOILERS].")
    assert_parse("<p>this is</p><div class=\"spoiler\"><p>a block spoiler</p></div><p>.</p>", "this is\n\n[spoiler]\na block spoiler\n[/spoiler].")
    assert_parse("<p>this is</p><div class=\"spoiler\"><p>a block spoiler</p></div><p>.</p>", "this is\n\n[SPOILERS]\na block spoiler\n[/SPOILERS].")
    assert_parse("<div class=\"spoiler\"><p>this is a spoiler with no closing tag</p><p>new text</p></div>", "[spoiler]this is a spoiler with no closing tag\n\nnew text")
    assert_parse("<div class=\"spoiler\"><p>this is a spoiler with no closing tag<br>new text</p></div>", "[spoiler]this is a spoiler with no closing tag\nnew text")
    assert_parse("<div class=\"spoiler\"><p>this is a block spoiler with no closing tag</p></div>", "[spoiler]\nthis is a block spoiler with no closing tag")
    assert_parse("<div class=\"spoiler\"><p>this is <span class=\"spoiler\">a nested</span> spoiler</p></div>", "[spoiler]this is [spoiler]a nested[/spoiler] spoiler[/spoiler]")

    # assert_parse('<div class="spoiler"><h4>Blah</h4></div>', "[spoiler]\nh4. Blah\n[/spoiler]")
    assert_parse(%{<div class="spoiler"><h4>Blah\n[/spoiler]</h4></div>}, "[spoiler]\nh4. Blah\n[/spoiler]") # XXX wrong

    # assert_parse('<p>First sentence</p><p>[/spoiler] Second sentence.</p>', "First sentence\n\n[/spoiler] Second sentence.")
    assert_parse("<p>First sentence</p>\n\n[/spoiler] Second sentence.", "First sentence\n\n[/spoiler] Second sentence.") # XXX wrong

    assert_parse('<p>inline <em>foo</em></p><div class="spoiler"><p>blah blah</p></div>', "inline [i]foo\n\n[spoiler]blah blah[/spoiler]")
    assert_parse('<p>inline <span class="spoiler"> foo</span></p><div class="spoiler"><p>blah blah</p></div>', "inline [spoiler] foo\n\n[spoiler]blah blah[/spoiler]")
  end

  def test_sub_sup
    assert_parse("<p><sub>test</sub></p>", "[sub]test[/sub]")
    assert_parse("<p><sup>test</sup></p>", "[sup]test[/sup]")
    assert_parse("<p><sub><sub>test</sub></sub></p>", "[sub][sub]test[/sub][/sub]")
    assert_parse("<p><sup><sup>test</sup></sup></p>", "[sup][sup]test[/sup][/sup]")
  end

  def test_color
    %w(gen general art artist cont contributor copy copyright char character spec species inv invalid meta lor lore).each do |color|
      assert_color("<p><span class=\"dtext-color-", color)
    end
    assert_color("<p><span class=\"dtext-color\" style=\"color:", 'yellow')
    assert_color("<p><span class=\"dtext-color\" style=\"color:", "#ccc")
    assert_color("<p><span class=\"dtext-color\" style=\"color:", "#12345")
    assert_color("<p><span class=\"dtext-color\" style=\"color:", "#1a1")

    assert_parse('<ul><li><span class="dtext-color" style="color:lime">test</span> abc</li></ul>', "* [color=lime]test[/color] abc", allow_color: true)
    assert_parse('<h1><span class="dtext-color" style="color:lime">test</span></h1>', "h1.[color=lime]test[/color]", allow_color: true)
    # assert_parse('<span class="dtext-color" style="color:lime"><h1>test</h1></span>', "[color=lime]h1.test[/color]", allow_color: true)
  end

  def test_color_not_allowed
    assert_parse("<p>test</p>", "[color=invalid]test[/color]")
    assert_parse("<p>test</p>", "[color=#123456]test[/color]")

    assert_parse('<ul><li>test abc</li></ul>', "* [color=lime]test[/color] abc")
    assert_parse('<h1>test</h1>', "h1.[color=lime]test[/color]")
    # assert_parse('<h1>test</h1>', "[color=lime]h1.test[/color]")
  end

  def test_nested_inline_code
    assert_parse(%{<span class="inline-code">`what`</span>}, "`\\`what\\``")
  end

  def test_paragraphs
    assert_parse("<p>abc</p>", "abc")
  end

  def test_paragraphs_with_newlines_1
    assert_parse("<p>a<br>b<br>c</p>", "a\nb\nc")
  end

  def test_paragraphs_with_newlines_2
    assert_parse("<p>a</p><p>b</p>", "a\n\nb")
  end

  def test_headers
    assert_parse("<h1>header</h1>", "h1. header")
    assert_parse("<ul><li>a</li></ul><h1>header</h1><ul><li>list</li></ul>", "* a\n\nh1. header\n* list")
    assert_parse("<p>blah h1. blah</p>", "blah h1. blah")

    assert_parse('<p>text</p><h1>header</h1>', "text\nh1. header")
    assert_parse('<p><em>text</em></p><h1>header</h1>', "[i]text\nh1. header")
    assert_parse('<div class="spoiler"><p>text</p><h1>header</h1></div>', "[spoiler]text\nh1. header")
    assert_parse('<h1>header</h1><p>text</p>', "h1. header\ntext")
    assert_parse('<ul><li>one</li></ul><h1>header</h1>', "* one\nh1. header")
    assert_parse('<ul><li>one</li></ul><h1>header</h1><ul><li>two</li></ul>', "* one\nh1. header\n* two")
    assert_parse('<h1>header</h1><h2>header</h2>', "h1. header\nh2. header")

    assert_parse('<h1><em>header</em></h1><p>blah</p>', "h1. [i]header\nblah")
    assert_parse('<h1><span class="spoiler">header</span></h1><p>blah</p>', "h1. [spoiler]header\nblah")
    assert_parse('<h1><a rel="nofollow" class="dtext-link" href="http://example.com">http://example.com</a></h1><p>blah</p>', %{h1. http://example.com\nblah})
    assert_parse('<h1><a rel="nofollow" class="dtext-link dtext-external-link" href="http://example.com">example</a></h1><p>blah</p>', %{h1. "example":http://example.com\nblah})

    assert_parse('<blockquote><blockquote><h1>header</h1></blockquote></blockquote>', %{[quote]\n\n[quote]\n\nh1. header\n[/quote]\n\n[/quote]})

    assert_parse('<blockquote><h1>header</h1></blockquote><p>one<br>two</p>', %{[quote]\nh1. header\n[/quote]\none\ntwo})
  end

  def test_quote_blocks
    assert_parse('<blockquote><p>test</p></blockquote>', "[quote]\ntest\n[/quote]")

    assert_parse('<blockquote><p>test</p></blockquote>', "[quote]\ntest\n[/quote] ")
    assert_parse('<blockquote><p>test</p></blockquote><p>blah</p>', "[quote]\ntest\n[/quote] blah")
    assert_parse('<blockquote><p>test</p></blockquote><p>blah</p>', "[quote]\ntest\n[/quote] \nblah")
    assert_parse('<blockquote><p>test</p></blockquote><p>blah</p>', "[quote]\ntest\n[/quote]\nblah")
    assert_parse('<blockquote><p>test</p></blockquote><p> blah</p>', "[quote]\ntest\n[/quote]\n blah") # XXX should ignore space

    assert_parse('<p>test<br>[/quote] blah</p>', "test\n[/quote] blah")
    assert_parse('<p>test<br>[/quote]</p><ul><li>blah</li></ul>', "test\n[/quote]\n* blah")

    assert_parse('<blockquote><p>test</p></blockquote><h4>See also</h4>', "[quote]\ntest\n[/quote]\nh4. See also")
    assert_parse('<blockquote><p>test</p></blockquote><div class="spoiler"><p>blah</p></div>', "[quote]\ntest\n[/quote]\n[spoiler]blah[/spoiler]")

    assert_parse("<p>inline </p><blockquote><p>blah blah</p></blockquote>", "inline [quote]blah blah[/quote]")
    assert_parse("<p>inline <em>foo </em></p><blockquote><p>blah blah</p></blockquote>", "inline [i]foo [quote]blah blah[/quote]")
    assert_parse('<p>inline <span class="spoiler">foo </span></p><blockquote><p>blah blah</p></blockquote>', "inline [spoiler]foo [quote]blah blah[/quote]")

    assert_parse("<p>inline <em>foo</em></p><blockquote><p>blah blah</p></blockquote>", "inline [i]foo\n\n[quote]blah blah[/quote]")
    assert_parse('<p>inline <span class="spoiler">foo </span></p><blockquote><p>blah blah</p></blockquote>', "inline [spoiler]\n\nfoo [quote]blah blah[/quote]")
  end

  def test_quote_blocks_with_list
    assert_parse("<blockquote><ul><li>hello</li><li>there</li></ul></blockquote><p>abc</p>", "[quote]\n* hello\n* there\n[/quote]\nabc")
    assert_parse("<blockquote><ul><li>hello</li><li>there</li></ul></blockquote><p>abc</p>", "[quote]\n* hello\n* there\n\n[/quote]\nabc")
  end

  def test_quote_with_unclosed_tags
    assert_parse('<blockquote><p><strong>foo</strong></p></blockquote>', "[quote][b]foo[/quote]")
    assert_parse('<blockquote><blockquote><p>foo</p></blockquote></blockquote>', "[quote][quote]foo[/quote]")
    assert_parse('<blockquote><div class="spoiler"><p>foo</p></div></blockquote>', "[quote][spoiler]foo[/quote]")
    assert_parse('<blockquote><pre>foo[/quote]</pre></blockquote>', "[quote][code]foo[/quote]")
    assert_parse('<blockquote><details><summary></summary><div><p>foo</p></div></details></blockquote>', "[quote][section]foo[/quote]")
    assert_parse('<blockquote><table class="striped"><td>foo</td></table></blockquote>', "[quote][table][td]foo[/quote]")
    assert_parse('<blockquote><ul><li>foo</li></ul></blockquote>', "[quote]* foo[/quote]")
    assert_parse('<blockquote><h1>foo</h1></blockquote>', "[quote]h1. foo[/quote]")
  end

  def test_quote_blocks_nested
    assert_parse("<blockquote><p>a</p><blockquote><p>b</p></blockquote><p>c</p></blockquote>", "[quote]\na\n[quote]\nb\n[/quote]\nc\n[/quote]")
  end

  def test_quote_blocks_nested_spoiler
    assert_parse("<blockquote><p>a<br><span class=\"spoiler\">blah</span><br>c</p></blockquote>", "[quote]\na\n[spoiler]blah[/spoiler]\nc[/quote]")
    assert_parse("<blockquote><p>a</p><div class=\"spoiler\"><p>blah</p></div><p>c</p></blockquote>", "[quote]\na\n\n[spoiler]blah[/spoiler]\n\nc[/quote]")
    assert_parse('<details><summary></summary><div><div class="spoiler"><ul><li>blah</li></ul></div></div></details>', "[section]\n[spoiler]\n* blah\n[/spoiler]\n[/section]")

    assert_parse('<details><summary></summary><div><div class="spoiler"><ul><li>blah</li></ul></div></div></details>', "[section]\n[spoiler]\n* blah\n[/spoiler]\n[/section]")
  end

  def test_quote_blocks_nested_expand
    assert_parse("<blockquote><p>a</p><details><summary></summary><div><p>b</p></div></details><p>c</p></blockquote>", "[quote]\na\n[section]\nb\n[/section]\nc\n[/quote]")
  end

  def test_code
    assert_parse("<pre>for (i=0; i&lt;5; ++i) {\n  printf(1);\n}\n\nexit(1);</pre>", "[code]for (i=0; i<5; ++i) {\n  printf(1);\n}\n\nexit(1);")

    assert_parse("<p>inline</p><pre>[/i]\n</pre>", "inline\n\n[code]\n[/i]\n[/code]")
    assert_parse('<p>inline</p><pre>[/i]</pre>', "inline\n\n[code][/i][/code]")
    assert_parse("<p><em>inline</em></p><pre>[/i]\n</pre>", "[i]inline\n\n[code]\n[/i]\n[/code]")
    assert_parse('<p><em>inline</em></p><pre>[/i]</pre>', "[i]inline\n\n[code][/i][/code]")
  end

  def test_urls
    assert_parse('<p>a <a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a> b</p>', 'a http://test.com b')
  end

  def test_urls_case_insensitive
    assert_parse('<p>a <a rel="nofollow" class="dtext-link" href="Https://test.com">Https://test.com</a> b</p>', 'a Https://test.com b')
  end

  def test_urls_with_newline
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a><br>b</p>', "http://test.com\nb")
  end

  def test_urls_with_paths
    assert_parse('<p>a <a rel="nofollow" class="dtext-link" href="http://test.com/~bob/image.jpg">http://test.com/~bob/image.jpg</a> b</p>', 'a http://test.com/~bob/image.jpg b')
  end

  def test_urls_with_fragment
    assert_parse('<p>a <a rel="nofollow" class="dtext-link" href="http://test.com/home.html#toc">http://test.com/home.html#toc</a> b</p>', 'a http://test.com/home.html#toc b')
  end

  def test_urls_with_params
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="https://test.com/?a=b&amp;c=d#abc">https://test.com/?a=b&amp;c=d#abc</a></p>', "https://test.com/?a=b&c=d#abc")
  end

  def test_auto_urls
    assert_parse('<p>a <a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a>. b</p>', 'a http://test.com. b')
  end

  def test_auto_urls_in_parentheses
    assert_parse('<p>a (<a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a>) b</p>', 'a (http://test.com) b')
  end

  def test_old_style_links
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com">test</a></p>', '"test":http://test.com')
  end

  def test_old_style_links_with_inline_tags
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com"><em>test</em></a></p>', '"[i]test[/i]":http://test.com')
  end

  def test_old_style_links_with_nested_links
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com">post #1</a></p>', '"post #1":http://test.com')
  end

  def test_old_style_links_with_special_entities
    assert_parse('<p>&quot;1&quot; <a rel="nofollow" class="dtext-link dtext-external-link" href="http://three.com">2 &amp; 3</a></p>', '"1" "2 & 3":http://three.com')
  end

  def test_new_style_links
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com">test</a></p>', '"test":[http://test.com]')
  end

  def test_new_style_links_with_inline_tags
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com/(parentheses)"><em>test</em></a></p>', '"[i]test[/i]":[http://test.com/(parentheses)]')
  end

  def test_new_style_links_with_nested_links
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com">post #1</a></p>', '"post #1":[http://test.com]')
  end

  def test_new_style_links_with_parentheses
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com/(parentheses)">test</a></p>', '"test":[http://test.com/(parentheses)]')
    assert_parse('<p>(<a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com/(parentheses)">test</a>)</p>', '("test":[http://test.com/(parentheses)])')
    assert_parse('<p>[<a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com/(parentheses)">test</a>]</p>', '["test":[http://test.com/(parentheses)]]')
  end

  def test_fragment_only_urls
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="#toc">test</a></p>', '"test":#toc')
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="#toc">test</a></p>', '"test":[#toc]')
  end

  def test_auto_url_boundaries
    assert_parse('<p>a （<a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a>） b</p>', 'a （http://test.com） b')
    assert_parse('<p>a 〜<a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a>〜 b</p>', 'a 〜http://test.com〜 b')
    assert_parse('<p>a <a rel="nofollow" class="dtext-link" href="http://test.com">http://test.com</a>　 b</p>', 'a http://test.com　 b')
  end

  def test_old_style_link_boundaries
    assert_parse('<p>a 「<a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com">title</a>」 b</p>', 'a 「"title":http://test.com」 b')
  end

  def test_new_style_link_boundaries
    assert_parse('<p>a 「<a rel="nofollow" class="dtext-link dtext-external-link" href="http://test.com">title</a>」 b</p>', 'a 「"title":[http://test.com]」 b')
  end

  def test_lists
    assert_parse('<ul><li>a</li></ul>', '* a')
    assert_parse('<ul><li>a</li><li>b</li></ul>', "* a\n* b")
    assert_parse('<ul><li>a</li><li>b</li><li>c</li></ul>', "* a\n* b\n* c")
    assert_parse('<ul><li>a</li></ul><p> </p>', "* a\n ") # XXX should strip space

    assert_parse('<ul><li>a</li><li>b</li></ul>', "* a\r\n* b")
    assert_parse('<ul><li>a</li></ul><ul><li>b</li></ul>', "* a\n\n* b")
    assert_parse('<ul><li>a</li><li>b</li><li>c</li></ul>', "* a\r\n* b\r\n* c")

    assert_parse('<ul><li>a</li><ul><li>b</li></ul></ul>', "* a\n** b")
    assert_parse('<ul><li>a</li><ul><li>b</li><ul><li>c</li></ul></ul></ul>', "* a\n** b\n*** c")
    # assert_parse('<ul><ul><ul><li>a</li></ul><li>b</li></ul><li>c</li></ul>', "*** a\n**\n b\n* c")
    assert_parse('<ul><ul><ul><li>a</li></ul></ul><li>b</li></ul>', "*** a\n* b")
    assert_parse('<ul><ul><ul><li>a</li></ul></ul></ul>', "*** a")

    assert_parse('<ul><li>a</li></ul><p>b</p><ul><li>c</li></ul>', "* a\nb\n* c")

    assert_parse('<p>a<br>b</p><ul><li>c</li><li>d</li></ul>', "a\nb\n* c\n* d")
    assert_parse('<p>a</p><ul><li>b</li></ul><p>c</p><ul><li>d</li></ul><p>e</p><p>another one</p>', "a\n* b\nc\n* d\ne\n\nanother one")
    assert_parse('<p>a</p><ul><li>b</li></ul><p>c</p><ul><ul><li>d</li></ul></ul><p>e</p><p>another one</p>', "a\n* b\nc\n** d\ne\n\nanother one")

    assert_parse('<ul><li><a class="dtext-link dtext-id-link dtext-post-id-link" href="/posts/1">post #1</a></li></ul>', "* post #1")

    assert_parse('<ul><li><em>a</em></li><li>b</li></ul>', "* [i]a[/i]\n* b")
    assert_parse('<ul><li><em>a</em></li><li>b</li></ul>', "* [i]a\n* b")
    assert_parse('<p><em>a</em></p><ul><li>b</li><li>c</li></ul>', "[i]a\n* b\n* c")

    # assert_parse('<ul><li></li></ul><h4>See also</h4><ul><li>a</li></ul>', "* h4. See also\n* a")
    assert_parse('<ul><li>h4. See also</li><li>a</li></ul>', "* h4. See also\n* a") # XXX wrong?

    assert_parse('<ul><li>a</li></ul><h4>See also</h4>', "* a\nh4. See also")
    assert_parse('<h4><em>See also</em></h4><ul><li>a</li></ul>', "h4. [i]See also\n* a")
    assert_parse('<ul><li><em>a</em></li></ul><h4>See also</h4>', "* [i]a\nh4. See also")

    assert_parse('<h4>See also</h4><ul><li>a</li></ul>', "h4. See also\n* a")
    assert_parse('<h4>See also</h4><ul><li>a</li><li>h4. External links</li></ul>', "h4. See also\n* a\n* h4. External links")

    # assert_parse('<p>a</p><div class="spoiler"><ul><li>b</li><li>c</li></ul></div><p>d</p>', "a\n[spoilers]\n* b\n* c\n[/spoilers]\nd")

    assert_parse('<p>a</p><blockquote><ul><li>b</li><li>c</li></ul></blockquote><p>d</p>', "a\n[quote]\n* b\n* c\n[/quote]\nd")
    assert_parse('<p>a</p><details><summary></summary><div><ul><li>b</li><li>c</li></ul></div></details><p>d</p>', "a\n[section]\n* b\n* c\n[/section]\nd")

    assert_parse('<p>a</p><blockquote><ul><li>b</li><li>c</li></ul><p>d</p></blockquote>', "a\n[quote]\n* b\n* c\n\nd")
    assert_parse('<p>a</p><details><summary></summary><div><ul><li>b</li><li>c</li></ul><p>d</p></div></details>', "a\n[section]\n* b\n* c\n\nd")

    assert_parse('<p>*</p>', "*")
    assert_parse('<p>*a</p>', "*a")
    assert_parse('<p>***</p>', "***")
    assert_parse('<p>*<br>*<br>*</p>', "*\n*\n*")
    assert_parse('<p>* <br>blah</p>', "* \r\nblah")
  end

  def test_inline_tags
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=tag">tag</a></p>', "{{tag}}")
    assert_parse('<p>hello </p><pre>tag</pre>', "hello [code]tag[/code]")
  end

  def test_inline_tags_conjunction
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=tag1%20tag2">tag1 tag2</a></p>', "{{tag1 tag2}}")
  end

  def test_inline_tags_special_entities
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=%3C3">&lt;3</a></p>', "{{<3}}")
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=%20%22%23%26%2B%3C%3E%3F"> &quot;#&amp;+&lt;&gt;?</a></p>', '{{ "#&+<>?}}')
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=%E6%9D%B1%E6%96%B9">東方</a></p>', "{{東方}}")
  end

  def test_inline_tags_lowercase_utf8
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=n%20%CE%A9%20p">n Ω P</a></p>', "{{n Ω P}}")
  end

  def test_inline_tags_aliased
    assert_parse('<p><a rel="nofollow" class="dtext-link dtext-post-search-link" href="/posts?tags=fox%20smile">Best Search</a></p>', "{{fox smile|Best Search}}")
  end

  def test_extra_newlines
    assert_parse('<p>a</p><p>b</p>', "a\n\n\n\n\n\n\nb\n\n\n\n")
  end

  def test_complex_links_1
    assert_parse("<p><a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=1\">2 3</a> | <a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=4\">5 6</a></p>", "[[1|2 3]] | [[4|5 6]]")
  end

  def test_complex_links_2
    assert_parse("<p>Tags <strong>(<a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=howto%3Atag\">Tagging Guidelines</a> | <a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=howto%3Atag_checklist\">Tag Checklist</a> | <a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=tag_groups\">Tag Groups</a>)</strong></p>", "Tags [b]([[howto:tag|Tagging Guidelines]] | [[howto:tag_checklist|Tag Checklist]] | [[Tag Groups]])[/b]")
  end

  def test_anchored_wiki_link
    assert_parse("<p><a rel=\"nofollow\" class=\"dtext-link dtext-wiki-link\" href=\"/wiki_pages/show_or_new?title=avoid_posting#a\">ABC 123</a></p>", "[[avoid_posting#A|ABC 123]]")
  end

  def test_internal_anchor
    assert_parse("<p><a id=\"b\"></a></p>", "[#B]")

    assert_parse("<p>[#test.abc]</p>", "[#test.abc]")
  end

  def text_note_id_link
    assert_parse('<p><a class="dtext-link dtext-id-link dtext-note-id-link" href="/notes/1234">note #1234</a></p>', "note #1234")
  end

  def test_table
    assert_parse("<table class=\"striped\"><tr><td>text</td></tr></table>", "[table][tr][td]text[/td][/tr][/table]")

    assert_parse("<table class=\"striped\"><thead><tr><th>header</th></tr></thead><tbody><tr><td><a class=\"dtext-link dtext-id-link dtext-post-id-link\" href=\"/posts/100\">post #100</a></td></tr></tbody></table>", "[table][thead][tr][th]header[/th][/tr][/thead][tbody][tr][td]post #100[/td][/tr][/tbody][/table]")
    assert_parse("<table class=\"striped\"><thead><tr><th>header</th></tr></thead><tbody><tr><td><a class=\"dtext-link dtext-id-link dtext-post-id-link\" href=\"/posts/100\">post #100</a></td></tr></tbody></table>", "[table]\n[thead]\n[tr]\n[th]header[/th][/tr][/thead][tbody][tr][td]post #100[/td][/tr][/tbody][/table]")

    assert_parse('<p>inline</p><table class="striped"><tr><td>text</td></tr></table>', "inline\n\n[table][tr][td]text[/td][/tr][/table]")
    assert_parse("<p><em>inline</em></p><table class=\"striped\"><tr><td>text</td></tr></table>", "[i]inline\n\n[table][tr][td]text[/td][/tr][/table]")
  end

  def test_unclosed_th
    assert_parse('<table class="striped"><th>foo</th></table>', "[table][th]foo")
  end

  def test_id_links
    assert_parse_id_link("dtext-post-id-link", "/posts/1234", "post #1234")
    assert_parse_id_link("dtext-post-changes-for-id-link", "/post_versions?search[post_id]=1234", "post changes #1234")
    assert_parse_id_link("dtext-post-flag-id-link", "/post_flags/1234", "flag #1234")
    assert_parse_id_link("dtext-note-id-link", "/notes/1234", "note #1234")
    assert_parse_id_link("dtext-forum-post-id-link", "/forum_posts/1234", "forum #1234")
    assert_parse_id_link("dtext-forum-topic-id-link", "/forum_topics/1234", "topic #1234")
    assert_parse_id_link("dtext-comment-id-link", "/comments/1234", "comment #1234")
    assert_parse_id_link("dtext-pool-id-link", "/pools/1234", "pool #1234")
    assert_parse_id_link("dtext-user-id-link", "/users/1234", "user #1234")
    assert_parse_id_link("dtext-artist-id-link", "/artists/1234", "artist #1234")
    assert_parse_id_link("dtext-ban-id-link", "/bans/1234", "ban #1234")
    assert_parse_id_link("dtext-tag-alias-id-link", "/tag_aliases/1234", "alias #1234")
    assert_parse_id_link("dtext-tag-implication-id-link", "/tag_implications/1234", "implication #1234")
    assert_parse_id_link("dtext-mod-action-id-link", "/mod_actions/1234", "mod action #1234")
    assert_parse_id_link("dtext-user-feedback-id-link", "/user_feedbacks/1234", "record #1234")
    assert_parse_id_link("dtext-blip-id-link", "/blips/1234", "blip #1234")
    assert_parse_id_link("dtext-set-id-link", "/post_sets/1234", "set #1234")
    assert_parse_id_link("dtext-takedown-id-link", "/takedowns/1234", "takedown #1234")
    assert_parse_id_link("dtext-takedown-id-link", "/takedowns/1234", "take down #1234", display: "takedown #1234")
    assert_parse_id_link("dtext-takedown-id-link", "/takedowns/1234", "takedown request #1234", display: "takedown #1234")
    assert_parse_id_link("dtext-takedown-id-link", "/takedowns/1234", "take down request #1234", display: "takedown #1234")
    assert_parse_id_link("dtext-ticket-id-link", "/tickets/1234", "ticket #1234")
    assert_parse_id_link("dtext-wiki-page-id-link", "/wiki_pages/1234", "wiki #1234")
  end

  def test_boundary_exploit
    assert_parse('<p>@mack&lt;</p>', "@mack<")
  end

  def test_expand
    assert_parse("<details><summary></summary><div><p>hello world</p></div></details>", "[section]hello world[/section]")

    assert_parse("<p>inline <em>foo </em></p><details><summary></summary><div><p>blah blah</p></div></details>", "inline [i]foo [section]blah blah[/section]")
    assert_parse('<p>inline <span class="spoiler">foo </span></p><details><summary></summary><div><p>blah blah</p></div></details>', "inline [spoiler]foo [section]blah blah[/section]")

    assert_parse("<p>inline <em>foo</em></p><details><summary></summary><div><p>blah blah</p></div></details>", "inline [i]foo\n\n[section]blah blah[/section]")
    assert_parse('<p>inline <span class="spoiler">foo</span></p><details><summary></summary><div><p>blah blah</p></div></details>', "inline [spoiler]foo\n\n[section]blah blah[/section]")

    assert_parse("<p>inline </p><details><summary></summary><div><p>blah blah</p></div></details>", "inline [section]blah blah[/section]")

    assert_parse('<details><summary></summary><div><p>test</p></div></details>', "[section]\ntest\n[/section] ")
    assert_parse('<details><summary></summary><div><p>test</p></div></details><p>blah</p>', "[section]\ntest\n[/section] blah")
    assert_parse('<details><summary></summary><div><p>test</p></div></details><p>blah</p>', "[section]\ntest\n[/section] \nblah")
    assert_parse('<details><summary></summary><div><p>test</p></div></details><p>blah</p>', "[section]\ntest\n[/section]\nblah")
    assert_parse('<details><summary></summary><div><p>test</p></div></details><p> blah</p>', "[section]\ntest\n[/section]\n blah") # XXX should ignore space

    assert_parse("<p>[/section]</p>", "[/section]")
    assert_parse("<p>foo [/section] bar</p>", "foo [/section] bar")
    assert_parse('<p>test<br>[/section] blah</p>', "test\n[/section] blah")
    assert_parse('<p>test<br>[/section]</p><ul><li>blah</li></ul>', "test\n[/section]\n* blah")

    assert_parse('<details><summary></summary><div><p>test</p></div></details><h4>See also</h4>', "[section]\ntest\n[/section]\nh4. See also")
    assert_parse('<details><summary></summary><div><p>test</p></div></details><div class="spoiler"><p>blah</p></div>', "[section]\ntest\n[/section]\n[spoiler]blah[/spoiler]")
  end

  def test_expand_missing_close
    assert_parse("<details><summary></summary><div><p>a</p></div></details>", "[section]a")
  end

  def test_aliased_expand
    assert_parse("<details><summary>hello</summary><div><p>blah blah</p></div></details>", "[section=hello]blah blah[/section]")

    assert_parse("<p>inline <em>foo </em></p><details><summary>title</summary><div><p>blah blah</p></div></details>", "inline [i]foo [section=title]blah blah[/section]")
    assert_parse('<p>inline <span class="spoiler">foo </span></p><details><summary>title</summary><div><p>blah blah</p></div></details>', "inline [spoiler]foo [section=title]blah blah[/section]")
  end

  def test_expand_with_nested_code
    assert_parse("<details><summary></summary><div><pre>hello\n</pre></div></details>", "[section]\n[code]\nhello\n[/code]\n[/section]")
  end

  def test_expand_with_nested_list
    assert_parse("<details><summary></summary><div><ul><li>a</li><li>b</li></ul></div></details><p>c</p>", "[section]\n* a\n* b\n[/section]\nc")
  end

  def test_inline_mode
    assert_equal("hello", DText.parse("hello", inline: true)[0].strip)
  end

  def test_old_asterisks
    assert_parse("<p>hello *world* neutral</p>", "hello *world* neutral")
  end

  def test_utf8_links
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="/posts?tags=approver:葉月">7893</a></p>', '"7893":/posts?tags=approver:葉月')
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="/posts?tags=approver:葉月">7893</a></p>', '"7893":[/posts?tags=approver:葉月]')
    assert_parse('<p><a rel="nofollow" class="dtext-link" href="http://danbooru.donmai.us/posts?tags=approver:葉月">http://danbooru.donmai.us/posts?tags=approver:葉月</a></p>', 'http://danbooru.donmai.us/posts?tags=approver:葉月')
  end

  def test_delimited_links
    dtext = '(blah <https://en.wikipedia.org/wiki/Orange_(fruit)>).'
    html = '<p>(blah <a rel="nofollow" class="dtext-link" href="https://en.wikipedia.org/wiki/Orange_(fruit)">https://en.wikipedia.org/wiki/Orange_(fruit)</a>).</p>'
    assert_parse(html, dtext)
  end

  def test_null_bytes
    e = assert_raises(DText::Error) { DText.parse("foo\0bar") }
    assert_equal("invalid byte sequence in UTF-8", e.message)
  end

  def test_wiki_link_xss
    e = assert_raises(DText::Error) do
      DText.parse("[[\xFA<script \xFA>alert(42); //\xFA</script \xFA>]]")
    end
    assert_equal("invalid byte sequence in UTF-8", e.message)
  end

  def test_mention_xss
    e = assert_raises(DText::Error) do
      DText.parse("@user\xF4<b>xss\xFA</b>")
    end
    assert_equal("invalid byte sequence in UTF-8", e.message)
  end

  def test_url_xss
    e = assert_raises(DText::Error) do
      DText.parse(%("url":/page\xF4">x\xFA<b>xss\xFA</b>))
    end
    assert_equal("invalid byte sequence in UTF-8", e.message)
  end
end

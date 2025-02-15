/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/compiler/whisker/detail/overload.h>
#include <thrift/compiler/whisker/detail/string.h>
#include <thrift/compiler/whisker/expected.h>
#include <thrift/compiler/whisker/lexer.h>
#include <thrift/compiler/whisker/parser.h>

#include <fmt/core.h>

#include <cassert>
#include <cstddef>
#include <iterator>
#include <map>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace whisker {

namespace {

/**
 * A parser_scan_window represents a single pass through lexed tokens by the
 * parser.
 *
 * The start and head of the parser_scan_window are used to track the current
 * position in the input lexed token stream. start represents the beginning of
 * an AST node, while the head is the current position of the parser.
 * parser_scan_window provides methods to advance the head position, peek at the
 * next token, and create new scan windows with different start or head
 * positions.
 *
 * The head advances as parsing progresses, and the start is moved up to the
 * head after each AST node is produced.
 *
 * parser_scan_window is copyable which means that backtracking can be achieved
 * by advancing copies of the window, then disposing the copy if parsing fails.
 */
struct parser_scan_window {
  using cursor = std::vector<token>::const_iterator;

  cursor start;
  cursor head;
  cursor end;

  parser_scan_window(cursor start, cursor head, cursor end)
      : start(start), head(head), end(end) {
    assert(start <= head);
    assert(head < end);
  }

  /**
   * Determines if the head is at the terminal token or not.
   * If this returns false, then both advance() and peek() will return the
   * terminal token in the lex stream.
   */
  bool can_advance() const { return std::next(head) < end; }
  /**
   * Advances the head of the scan by one and returns the encountered
   * token. If the head is already at the end of the input, then there is
   * no changed state and the terminal token is returned.
   */
  const token& advance() {
    if (!can_advance()) {
      // Don't advance past the terminal token (end of lex stream)
      return *head;
    }
    return *head++;
  }
  /**
   * Creates a copy of this parser_scan_window and advances the head n times.
   */
  [[nodiscard]] parser_scan_window next(std::size_t n = 1) {
    return with_head(std::min(std::prev(end), std::next(head, n)));
  }

  /**
   * Returns the next token without advancing the head to the next token.
   */
  [[nodiscard]] const token& peek() const { return *head; }

  /**
   * Returns a new "fresh" parser_scan_window whose start is moved to the
   * current head. Typically, this indicates that the tokens in the current
   * parser_scan_window have been consumed. The result is an empty
   * parser_scan_window.
   */
  [[nodiscard]] parser_scan_window make_fresh() const {
    return with_start(head);
  }
  [[nodiscard]] bool empty() const { return start == head; }

  [[nodiscard]] parser_scan_window with_start(cursor new_start) const {
    assert(new_start <= head);
    return parser_scan_window(new_start, head, end);
  }
  [[nodiscard]] parser_scan_window with_head(cursor new_head) const {
    assert(start <= new_head);
    assert(new_head < end);
    return parser_scan_window(start, new_head, end);
  }

  source_location start_location() const { return start->range.begin; }
  source_location head_location() const { return head->range.begin; }
  source_location end_location() const {
    return empty() ? start_location() : std::prev(head)->range.end;
  }
  source_range range() const { return {start_location(), end_location()}; }
};

/**
 * Result of a scan which resulted in a parsed representation of tokens.
 * The (now advanced) parser_scan_window is packaged with the token so the
 * parser can move up its cursor.
 */
template <typename T>
struct parsed_object {
  parsed_object(T value, const parser_scan_window& advanced)
      : value_{std::move(value)}, new_head_{advanced.make_fresh()} {}

  /**
   * Advances the provided parser_scan_window to the last consumed token as part
   * of parsing, then returns the result of parsing. This method ensures that
   * the result of parsing cannot be consumed without advancing the cursor.
   */
  [[nodiscard]] T consume_and_advance(parser_scan_window* scan) && {
    assert(scan);
    *scan = std::move(new_head_);
    return std::move(value_);
  }

 private:
  T value_;
  parser_scan_window new_head_;
};

/**
 * A marker struct that indicates that parsing failed (non-fatally).
 */
struct no_parse_result {};

template <typename T>
struct [[nodiscard]] parse_result
    : private expected<parsed_object<T>, no_parse_result> {
 private:
  using base = expected<parsed_object<T>, no_parse_result>;

 public:
  parse_result(T value, const parser_scan_window& advanced)
      : base(std::in_place, std::move(value), advanced) {}
  /* implicit */ parse_result(no_parse_result) : base(unexpect) {}

  [[nodiscard]] T consume_and_advance(parser_scan_window* scan) && {
    assert(this->has_value());
    return std::move(**this).consume_and_advance(scan);
  }

  using base::operator bool;
  using base::has_value;
  using base::operator*;
  using base::operator->;
};

bool try_consume_token(parser_scan_window* scan, tok kind) {
  assert(scan);
  if (scan->peek().kind == kind) {
    scan->advance();
    return true;
  }
  return false;
}

/**
 * The Mustache spec contains this concept called "standalone lines".
 *   https://github.com/mustache/spec/blob/v1.4.2/specs/sections.yml#L279-L305
 *
 * If a line opens a section block, and the rest of the line is whitespace only,
 * then that whitespace is stripped from the output.
 *
 * Consider the following template:
 *
 *     {{#true_value}}
 *       hello
 *     {{/true_value}}
 *
 * This will output "  hello\n". Notice that the newline following
 * "{{#true_value}}" is stripped. However, the newline after "hello" is
 * retained.
 *
 * If we have a {{variable}} then we do not see such behavior:
 *
 *     {{#true_value}} {{foo}}
 *       hello
 *     {{/true_value}}
 *
 * This will output " \n  hello\n".
 *
 * Things get even weirder because the definition of a "line" is quite liberal.
 * Consider the following template:
 *
 *     | This Is
 *       {{#boolean
 *            .condition}}
 *     |
 *       {{/boolean.condition}}
 *     | A Line
 *
 * This will output: "| This Is\n| A Line\n". In other words, the
 * {{#boolean.condition}} "line" should be stripped of whitespace even though
 * the whitespace spans multiple lines. This is because "{{ }}" eats whitespace
 * and newlines can only appear in textual content.
 *
 * Partials are also a special case, for three reasons:
 *   1. They can be contextually standalone even though they are not invisible.
 *   2. They cannot be combined with other standalone constructs. For example,
 *      two partial applications on the same line is not standalone.
 *   3. Standalone partial whitespace stripping applies only to its right side.
 *
 * Why does this belong here (in between the lexer and the parser)?
 *
 *   The standalone lines rules are particularly weird because it involves
 *   checking for blocks (a parsed AST concept) but also asserts that whitespace
 *   be removed (typically in the lexer's domain, not the parser).
 *
 *   In other words, the lexer is too "dumb" and the parser is too "refined".
 *   Therefore, this step can be considered a lexer post-process or a parser
 *   pre-process step.
 *
 * The implementation here is aimed to match mstch.
 */
class standalone_lines_scanner {
 public:
  /**
   * A marker struct indicating a tok::whitespace or tok::newline that should
   * be stripped because they are part of a standalone line.
   */
  struct removed {};
  /**
   * A marker struct indicating a block that is standalone.
   */
  struct standalone_block {};
  /**
   * A struct indicating a standalone partial application.
   */
  struct partial_apply {
    /**
     * The whitespace content to the left of the partial application that
     * should be replicated for subsequent lines in multi-line partial
     * applications.
     */
    std::string preceding_whitespace;
  };
  using standalone_marking =
      std::variant<removed, standalone_block, partial_apply>;
  /**
   * The cursors in the set point to either:
   *   - the "{{" token of the corresponding standalone construct.
   *   - tok::whitespace or tok::newline.
   *
   * Using std::map here because cursor is an iterator and thus has operator<
   * but not necessarily std::hash<>.
   */
  using result = std::map<parser_scan_window::cursor, standalone_marking>;

  /**
   * Returns a result containing the cursors of the tokens that are "involved"
   * in standalone constructs. The cursors can be used by the parser to remove
   * whitespace or add partial standalone offset information.
   */
  static result mark(const std::vector<token>& tokens) {
    assert(!tokens.empty());
    if (tokens.back().kind == tok::error) {
      // No point stripping lines
      return {};
    }

    parser_scan_window scan(
        tokens.cbegin() /* start */,
        tokens.cbegin() /* head */,
        tokens.cend() /* end */);

    using cursor = parser_scan_window::cursor;

    result marked_tokens;
    const auto& mark_removal = [&](cursor pos) {
      assert(pos->kind == tok::whitespace || pos->kind == tok::newline);
      [[maybe_unused]] const auto& [_, inserted] =
          marked_tokens.insert({pos, removed()});
      assert(inserted);
    };

    const auto& mark_standalone = [&](cursor pos) {
      assert(pos->kind == tok::open);
      [[maybe_unused]] const auto& [_, inserted] =
          marked_tokens.insert({pos, standalone_block()});
      assert(inserted);
    };

    const auto& mark_partial_apply = [&](cursor pos, partial_apply partial) {
      assert(pos->kind == tok::open);
      [[maybe_unused]] const auto& [_, inserted] =
          marked_tokens.insert({pos, std::move(partial)});
      assert(inserted);
    };

    struct line_info {
      cursor start;
      std::vector<std::pair<cursor, standalone_marking>> markings = {};
    };
    line_info current_line{scan.head};

    const auto begin_next_line = [&]() {
      current_line.markings.clear();
      current_line = {scan.head, std::move(current_line.markings)};
      scan = scan.make_fresh();
    };

    // Only after scanning the entire line can we be sure if there are eligible
    // standalone constructs. This function "commits" the changes aggregated in
    // the current line into the output.
    const auto commit_current_line_markings = [&]() {
      if (!current_line.markings.empty()) {
        for (cursor c = current_line.start; c < scan.head; ++c) {
          if (c->kind == tok::whitespace || c->kind == tok::newline) {
            mark_removal(c);
          }
        }
        for (auto& m : current_line.markings) {
          cursor c = m.first;
          auto& marking = m.second;
          detail::variant_match(
              std::move(marking),
              [&](standalone_block&&) { mark_standalone(c); },
              [&](partial_apply&& partial) {
                mark_partial_apply(c, std::move(partial));
              },
              [&](removed&&) {
                throw std::logic_error(
                    "Removal markings should only happen at the end of the line");
              });
        }
      }
      begin_next_line();
    };

    // When an ineligible line is detected, we drain the current line's tokens
    // and dump all the potential standalone candidates from the current line.
    const auto drain_current_line = [&]() {
      do {
        const auto& t = scan.advance();
        if (t.kind == tok::newline) {
          break;
        }
      } while (scan.can_advance());
      begin_next_line();
    };

    while (scan.can_advance()) {
      if (scan.peek().kind == tok::newline) {
        scan.advance();
        commit_current_line_markings();
      }

      if (try_consume_token(&scan, tok::whitespace)) {
        scan = scan.make_fresh();
        continue;
      }
      if (try_consume_token(&scan, tok::text)) {
        drain_current_line();
        continue;
      }
      if (parse_result standalone = parse_standalone_compatible(scan)) {
        auto scan_start = scan.start;
        switch (std::move(standalone).consume_and_advance(&scan)) {
          case standalone_compatible_kind::ineligible:
            drain_current_line();
            continue;
          case standalone_compatible_kind::partial_apply: {
            if (!current_line.markings.empty()) {
              // Even though we allow multiple standalone constructs on the same
              // line, for partials it does not apply
              drain_current_line();
              continue;
            }
            std::string preceding_whitespace;
            for (cursor c = current_line.start; c != scan_start; ++c) {
              assert(c->kind == tok::whitespace);
              preceding_whitespace += c->string_value();
            }
            current_line.markings.emplace_back(
                scan_start, partial_apply{std::move(preceding_whitespace)});
            // Do not strip the left side
            current_line.start = scan.head;
            break;
          }
          default:
            // These blocks strip both sides
            current_line.markings.emplace_back(scan_start, standalone_block());
            break;
        }
      } else {
        // This line is not eligible for stripping
        drain_current_line();
      }
    }

    // In case, last line is not terminated by a newline.
    // Otherwise, the current line is ineligible so nothing happens.
    commit_current_line_markings();

    return marked_tokens;
  }

  // A line containing any of the following constructs...
  //   - "{{! ... }}"
  //   - "{{# ... }}"
  //   - "{{^ ... }}"
  //   - "{{/ ... }}"
  //   - "{{> ... }}"
  //   - "{{else}}"
  // ..and ONLY those constructs are candidates for whitespace stripping. That's
  // because these kind of templates are invisible in the output — their purpose
  // is purely to express intent within the templating language.
  // The only exception is partial applications, where only the right side is
  // stripped when standalone.
  enum class standalone_compatible_kind {
    comment, // "{{!"
    block, // "{{#", "{{^", or "{{"/"}
    partial_apply, // "{{>"
    else_clause, // "{{else}}"
    ineligible // "{{variable}} for example
  };
  static parse_result<standalone_compatible_kind> parse_standalone_compatible(
      parser_scan_window scan) {
    assert(scan.empty());
    if (!try_consume_token(&scan, tok::open)) {
      return no_parse_result();
    }

    standalone_compatible_kind kind;
    switch (scan.advance().kind.value) {
      case tok::bang:
        kind = standalone_compatible_kind::comment;
        break;
      case tok::pound:
      case tok::caret:
      case tok::slash:
        kind = standalone_compatible_kind::block;
        break;
      case tok::gt:
        kind = standalone_compatible_kind::partial_apply;
        break;
      case tok::kw_else:
        kind = standalone_compatible_kind::else_clause;
        break;
      default:
        kind = standalone_compatible_kind::ineligible;
    }

    // Keep consuming tokens until we reach the end of the "{{ }}"... even if it
    // means going over to the next line. Templates are supposed to eat
    // whitespace.
    while (scan.can_advance()) {
      if (scan.advance().kind == tok::close) {
        break;
      }
    }
    return {kind, scan};
  }
};

/**
 * Recursive descent parser for Whisker templates that outputs
 * whisker::ast::root if successful.
 *
 * Parsing for production rules of the grammar are implemented as member
 * functions named parse_<rule>(). Each such method includes a specification of
 * the grammar being parsed.
 *
 * Parsing begins at the parse_root() function.
 *
 * The grammar is descibed with PEG (Parsing Expression Grammar) rules,
 * following a syntax closely resembling pest.rs:
 *   https://pest.rs/
 *
 * Production rules are of the form:
 *
 *     rule → { <term> <op> <term> ... }
 *
 * <term> represents other rules. <op> represents a combinator of rules. Here
 * are the supported set of combinators:
 *
 *     a ~ b — exactly one a followed by exactly one b.
 *     a* — zero or more repetitions of a.
 *     a+ — one or more repetitions of a.
 *     a? — exactly zero or one repetition of a.
 *     !a — assert that there is no match to a (without consuming input).
 *     a | b — exactly one a or one b (a matches first)
 *     (a <op> b) — parentheses to disambiguate groups of rules and combinators.
 */
class parser {
 private:
  std::vector<token> tokens_;
  diagnostics_engine& diags_;
  standalone_lines_scanner::result standalone_markings_;

  // Reports an error without failing the parse.
  template <typename... T>
  void report_error(
      const parser_scan_window& scan,
      fmt::format_string<T...> msg,
      T&&... args) {
    diags_.error(scan.head_location(), msg, std::forward<T>(args)...);
  }

  struct parse_error : std::exception {};

  template <typename... T>
  [[noreturn]] void report_fatal_error(
      const parser_scan_window& scan,
      fmt::format_string<T...> msg,
      T&&... args) {
    report_error(scan, msg, std::forward<T>(args)...);
    throw parse_error();
  }

  [[noreturn]] void report_expected(
      const parser_scan_window& scan, std::string_view expected) {
    report_fatal_error(
        scan,
        "expected {} but found {}",
        expected,
        to_string(scan.peek().kind));
  }

  [[nodiscard]] bool is_standalone_whitespace(
      parser_scan_window::cursor pos) const {
    assert(pos->kind == tok::whitespace || pos->kind == tok::newline);
    return standalone_markings_.find(pos) != standalone_markings_.end();
  }

  std::optional<std::string> standalone_partial_offset(
      parser_scan_window::cursor pos) const {
    assert(pos->kind == tok::open);
    if (auto marking = standalone_markings_.find(pos);
        marking != standalone_markings_.end()) {
      return std::get<standalone_lines_scanner::partial_apply>(marking->second)
          .preceding_whitespace;
    }
    return std::nullopt;
  }

  // root → { body* }
  std::optional<ast::root> parse_root(parser_scan_window scan) {
    constexpr std::string_view expected_types = "text, template, or comment";
    try {
      auto original_scan = scan;
      ast::bodies bodies;
      while (scan.can_advance()) {
        if (parse_result maybe_body = parse_body(scan)) {
          if (auto body = std::move(maybe_body).consume_and_advance(&scan)) {
            bodies.emplace_back(std::move(*body));
          }
        } else if (auto else_clause = parse_else_clause(scan)) {
          // "{{else}}" marks the end of a body and beginning of a new one,
          // which cannot happen at the root scope.
          report_fatal_error(
              scan,
              "expected {} but found dangling else-clause",
              expected_types);
        } else {
          report_expected(scan, expected_types);
        }
      }
      return ast::root{original_scan.start_location(), std::move(bodies)};
    } catch (const parse_error&) {
      // the error should already have been reported via the diagnostics
      // engine
      return std::nullopt;
    }
  }

  // Parses the "{{else}}" clause which is a special construct that looks like
  // variable interpolation but is actually a separator between two ast::bodies.
  //
  // else-clause → { "{{" ~ "else" ~ "}}" }
  parse_result<std::monostate> parse_else_clause(parser_scan_window scan) {
    if (!(try_consume_token(&scan, tok::open) &&
          try_consume_token(&scan, tok::kw_else) &&
          try_consume_token(&scan, tok::close))) {
      return no_parse_result();
    }
    return {{}, scan};
  }

  // Returns an empty parse result if no body was found.
  //
  // Returns an empty optional<ast::body> if body was found but consisted
  // entirely of stripped whitespace from a standalone construct, with the
  // advanced scan window.
  //
  // Returns the ast::body found otherwise.
  //
  // body → { text | newline | template | comment }
  parse_result<std::optional<ast::body>> parse_body(parser_scan_window scan) {
    assert(scan.empty());
    const auto scan_start = scan.head;

    std::optional<ast::body> body;
    while (!body.has_value()) {
      if (parse_result maybe_text = parse_text(scan)) {
        body = std::move(maybe_text).consume_and_advance(&scan);
      } else if (parse_result maybe_newline = parse_newline(scan)) {
        body = std::move(maybe_newline).consume_and_advance(&scan);
      } else if (parse_result else_clause = parse_else_clause(scan)) {
        // The "{{else}}" clause looks like variable interpolation so we should
        // capture it first before recursing into parse_template. The else
        // clause actually marks the end of the current block (and the beginning
        // of the next one).
        break;
      } else if (parse_result templ = parse_template(scan)) {
        detail::variant_match(
            std::move(templ).consume_and_advance(&scan),
            [&](auto&& t) { body = std::move(t); });
      } else if (parse_result comment = parse_comment(scan)) {
        body = std::move(comment).consume_and_advance(&scan);
      } else {
        // Next token is not valid for a body element
        break;
      }
    }

    if (scan.head == scan_start) {
      // We did not find any body elements. Not even standalone whitespace.
      return no_parse_result();
    }
    return {std::move(body), scan};
  }
  /**
   * Parses the grammar for "body*". If there are no bodies present, returns an
   * empty vector.
   */
  parsed_object<ast::bodies> parse_bodies(parser_scan_window scan) {
    ast::bodies bodies;
    while (parse_result maybe_body = parse_body(scan)) {
      if (auto body = std::move(maybe_body).consume_and_advance(&scan)) {
        bodies.emplace_back(std::move(*body));
      }
    }
    return {std::move(bodies), scan};
  }

  // Returns an empty parse result if no text was found.
  //
  // Returns an empty optional<ast::text> if text was found but completely
  // stripped because of standalone constructs, with the advanced scan window.
  //
  // Returns the ast::text found otherwise with a non-empty string.
  //
  // text → { (tok::text | whitespace)+ }
  parse_result<std::optional<ast::text>> parse_text(parser_scan_window scan) {
    assert(scan.empty());
    std::string result;
    while (scan.can_advance()) {
      const token& t = scan.peek();
      if (t.kind != tok::text && t.kind != tok::whitespace) {
        break;
      }
      bool is_stripped =
          t.kind == tok::whitespace && is_standalone_whitespace(scan.head);
      if (!is_stripped) {
        result += t.string_value();
      }
      scan.advance();
    }
    if (scan.head == scan.start) {
      // No text was scanned. Not even standalone whitespace.
      return no_parse_result();
    }
    if (result.empty()) {
      // Text was scanned but they were all stripped. We still need to advance
      // the scan window.
      return {std::nullopt, scan};
    }
    return parse_result{std::optional{ast::text{scan.range(), result}}, scan};
  }

  // Returns an empty parse result if no newline was found.
  //
  // Returns an empty optional<ast::newline> if newline was found but stripped
  // because of standalone constructs, with the advanced scan window.
  //
  // Returns the ast::newline found otherwise.
  //
  // newline → { tok::newline+ }
  parse_result<std::optional<ast::newline>> parse_newline(
      parser_scan_window scan) {
    assert(scan.empty());
    if (scan.peek().kind == tok::newline) {
      if (is_standalone_whitespace(scan.head)) {
        // Stripped because of a standalone construct
        return {std::nullopt, scan.next()};
      }
      const token& text = scan.advance();
      return {
          ast::newline{scan.range(), std::string(text.string_value())}, scan};
    }
    return no_parse_result();
  }

  // comment → { basic-comment | escaped-comment }
  // basic-comment → { "{{!" ~ <raw text until we see "}}"> ~ "}}" }
  // escaped-comment → { "{{!--" ~ <raw text until we see "--}}"> ~ "--}}" }
  //
  // NOTE: the difference between basic-comment and escaped-comment is dealt
  // with by the lexer already.
  parse_result<ast::comment> parse_comment(parser_scan_window scan) {
    assert(scan.empty());
    if (scan.advance().kind != tok::open) {
      return no_parse_result();
    }
    if (scan.advance().kind != tok::bang) {
      return no_parse_result();
    }
    const auto& text = scan.peek();
    switch (text.kind) {
      case tok::text:
        scan.advance();
        break;
      case tok::close:
        // empty comment
        scan.advance();
        return {ast::comment{scan.range(), ""}, scan};
      default:
        report_expected(scan, "comment text");
    }
    if (scan.peek().kind != tok::close) {
      report_expected(scan, fmt::format("{} to close comment", tok::close));
    }
    scan.advance();

    return {ast::comment{scan.range(), std::string(text.string_value())}, scan};
  }

  using template_body = std::variant<
      ast::variable,
      ast::section_block,
      ast::conditional_block,
      ast::partial_apply>;
  // template → { variable | section-block | partial-apply }
  parse_result<template_body> parse_template(parser_scan_window scan) {
    assert(scan.empty());
    if (scan.peek().kind != tok::open) {
      return no_parse_result();
    }
    switch (scan.next().peek().kind) {
      case tok::bang:
        // this is a comment so don't fail the parse
        [[fallthrough]];
      case tok::slash:
        // parse_template can be called recursively, which means that seeing
        // tok::slash implies that the parent node is closing (so don't fail the
        // parse).
        return no_parse_result();
      default:
        // continue parsing as a template or fail the parse!
        break;
    }

    std::optional<template_body> templ;
    if (parse_result variable = parse_variable(scan)) {
      templ = std::move(variable).consume_and_advance(&scan);
    } else if (parse_result conditional_block = parse_conditional_block(scan)) {
      templ = std::move(conditional_block).consume_and_advance(&scan);
    } else if (parse_result section_block = parse_section_block(scan)) {
      templ = std::move(section_block).consume_and_advance(&scan);
    } else if (parse_result partial_apply = parse_partial_apply(scan)) {
      templ = std::move(partial_apply).consume_and_advance(&scan);
    }
    if (!templ.has_value()) {
      report_expected(scan, "variable, block, or partial-apply in template");
    }
    return {std::move(*templ), scan};
  }

  // variable → { "{{" ~ variable-lookup ~ "}}" }
  parse_result<ast::variable> parse_variable(parser_scan_window scan) {
    assert(scan.empty());
    const auto scan_start = scan.start;

    if (!try_consume_token(&scan, tok::open)) {
      return no_parse_result();
    }
    switch (scan.peek().kind.value) {
      case tok::bang:
        // this is a comment
        [[fallthrough]];
      case tok::pound:
      case tok::caret:
        // this is a section-block-open
        [[fallthrough]];
      case tok::slash:
        // this is a section-block-close
        return no_parse_result();
      case tok::gt:
        // this is a partial-apply
        return no_parse_result();
      default:
        // continue parsing as a variable (and fail if it's not!)
        break;
    }
    scan = scan.make_fresh();

    parse_result variable_lookup = parse_variable_lookup(scan);
    if (!variable_lookup.has_value()) {
      report_expected(scan, "variable-lookup in variable");
    }
    ast::variable_lookup lookup =
        std::move(variable_lookup).consume_and_advance(&scan);
    if (!try_consume_token(&scan, tok::close)) {
      report_expected(scan, fmt::format("{} to close variable", tok::close));
    }
    return {
        ast::variable{scan.with_start(scan_start).range(), std::move(lookup)},
        scan};
  }

  // variable-lookup → { "." | (identifier ~ ("." ~ identifier)*) }
  parse_result<ast::variable_lookup> parse_variable_lookup(
      parser_scan_window scan) {
    assert(scan.empty());
    const auto scan_start = scan.start;
    if (try_consume_token(&scan, tok::dot)) {
      return {
          ast::variable_lookup{
              scan.with_start(scan_start).range(),
              ast::variable_lookup::this_ref{}},
          scan};
    }

    scan = scan.make_fresh();
    const token& first_id = scan.advance();
    if (first_id.kind != tok::identifier) {
      return no_parse_result();
    }
    std::vector<ast::identifier> path;
    path.emplace_back(
        ast::identifier{first_id.range, std::string(first_id.string_value())});

    while (try_consume_token(&scan, tok::dot)) {
      const token& id_part = scan.peek();
      if (id_part.kind != tok::identifier) {
        report_expected(scan, "identifier in variable-lookup");
      }
      scan.advance();
      path.emplace_back(
          ast::identifier{id_part.range, std::string(id_part.string_value())});
    }

    auto range = scan.with_start(scan_start).range();
    return {ast::variable_lookup{range, std::move(path)}, scan};
  }

  // section-block → { section-block-open ~ body* ~ section-block-close }
  // section-block-open → { "{{" ~ ("#" | "^") ~ variable-lookup ~ "}}" }
  // section-block-close → { "{{" ~ "/" ~ variable-lookup ~ "}}" }
  //
  // NOTE: the variable-lookups must match between open and close
  parse_result<ast::section_block> parse_section_block(
      parser_scan_window scan) {
    assert(scan.empty());
    const auto scan_start = scan.start;
    if (!try_consume_token(&scan, tok::open)) {
      return no_parse_result();
    }
    bool is_inverted = try_consume_token(&scan, tok::caret);
    if (!is_inverted) {
      if (!try_consume_token(&scan, tok::pound)) {
        // neither "#" nor "^" so this is not a section block
        return no_parse_result();
      }
    }

    parse_result lookup_at_open = parse_variable_lookup(scan.make_fresh());
    if (!lookup_at_open.has_value()) {
      report_expected(scan, "variable-lookup to open section-block");
    }
    ast::variable_lookup open =
        std::move(lookup_at_open).consume_and_advance(&scan);
    if (!try_consume_token(&scan, tok::close)) {
      report_expected(
          scan, fmt::format("{} to open section-block", tok::close));
    }
    scan = scan.make_fresh();

    ast::bodies bodies = parse_bodies(scan).consume_and_advance(&scan);

    if (!try_consume_token(&scan, tok::open)) {
      report_expected(
          scan,
          fmt::format(
              "{} to close section-block '{}'",
              tok::open,
              open.chain_string()));
    }
    if (!try_consume_token(&scan, tok::slash)) {
      report_expected(
          scan,
          fmt::format(
              "{} to close section-block '{}'",
              tok::slash,
              open.chain_string()));
    }

    parse_result lookup_at_close = parse_variable_lookup(scan.make_fresh());
    if (!lookup_at_close.has_value()) {
      report_expected(
          scan,
          fmt::format(
              "variable-lookup to close section-block '{}'",
              open.chain_string()));
    }
    ast::variable_lookup close =
        std::move(lookup_at_close).consume_and_advance(&scan);

    bool should_fail = false;
    if (open.chain_string() != close.chain_string()) {
      should_fail = true;
      report_error(
          scan,
          "section-block opening '{}' does not match closing '{}'",
          open.chain_string(),
          close.chain_string());
    }
    if (!try_consume_token(&scan, tok::close)) {
      should_fail = true;
      report_error(
          scan,
          "expected {} to close section-block '{}'",
          tok::close,
          open.chain_string());
    }
    if (should_fail) {
      throw parse_error();
    }

    return {
        ast::section_block{
            scan.with_start(scan_start).range(),
            is_inverted,
            std::move(open),
            std::move(bodies),
        },
        scan};
  }

  // conditional-block →
  //   { cond-block-open ~ body* ~ else-block? ~ cond-block-close }
  // cond-block-open →
  //   { "{{" ~ "#" ~ ("if" | "unless") ~ variable-lookup ~ "}}" }
  // else-block → { "{{" ~ "else" ~ "}}" ~ body* }
  // cond-block-close → { "{{" ~ "/" ~ ("if" | "unless") ~ "}}" }
  //
  // NOTE: the "if" or "unless" must match between open and close
  parse_result<ast::conditional_block> parse_conditional_block(
      parser_scan_window scan) {
    assert(scan.empty());
    const auto scan_start = scan.start;

    if (!(try_consume_token(&scan, tok::open) &&
          try_consume_token(&scan, tok::pound))) {
      return no_parse_result();
    }
    bool inverted;
    if (try_consume_token(&scan, tok::kw_if)) {
      inverted = false;
    } else if (try_consume_token(&scan, tok::kw_unless)) {
      inverted = true;
    } else {
      return no_parse_result();
    }
    scan = scan.make_fresh();

    const std::string_view block_name = inverted ? "unless" : "if";

    parse_result lookup = parse_variable_lookup(scan);
    if (!lookup.has_value()) {
      report_expected(
          scan, fmt::format("variable-lookup to open {}-block", block_name));
    }
    ast::variable_lookup open = std::move(lookup).consume_and_advance(&scan);
    if (!try_consume_token(&scan, tok::close)) {
      report_expected(
          scan, fmt::format("{} to open {}-block", tok::close, block_name));
    }
    scan = scan.make_fresh();

    ast::bodies bodies = parse_bodies(scan).consume_and_advance(&scan);

    auto else_ = std::invoke(
        [this,
         scan]() mutable -> parse_result<ast::conditional_block::else_block> {
          const auto else_scan_start = scan.start;
          if (parse_result e = parse_else_clause(scan)) {
            std::ignore = std::move(e).consume_and_advance(&scan);
          } else {
            return no_parse_result();
          }
          scan = scan.make_fresh();
          auto else_bodies = parse_bodies(scan).consume_and_advance(&scan);
          return {
              ast::conditional_block::else_block{
                  scan.with_start(else_scan_start).range(),
                  std::move(else_bodies)},
              scan};
        });
    auto else_block =
        std::invoke([&]() -> std::optional<ast::conditional_block::else_block> {
          if (else_.has_value()) {
            return std::move(else_).consume_and_advance(&scan);
          }
          return std::nullopt;
        });

    const auto expect_on_close = [&](tok kind) {
      if (!try_consume_token(&scan, kind)) {
        report_expected(
            scan,
            fmt::format(
                "{} to close {}-block '{}'",
                kind,
                block_name,
                open.chain_string()));
      }
    };
    expect_on_close(tok::open);
    expect_on_close(tok::slash);
    expect_on_close(inverted ? tok::kw_unless : tok::kw_if);
    expect_on_close(tok::close);

    return {
        ast::conditional_block{
            scan.with_start(scan_start).range(),
            inverted,
            std::move(open),
            std::move(bodies),
            std::move(else_block),
        },
        scan};
  }

  // partial-apply → { "{{" ~ ">" ~ partial-lookup ~ "}}" }
  parse_result<ast::partial_apply> parse_partial_apply(
      parser_scan_window scan) {
    assert(scan.empty());
    const auto scan_start = scan.start;

    if (!try_consume_token(&scan, tok::open)) {
      return no_parse_result();
    }
    if (!try_consume_token(&scan, tok::gt)) {
      return no_parse_result();
    }
    scan = scan.make_fresh();

    parse_result partial_lookup = parse_partial_lookup(scan.make_fresh());
    if (!partial_lookup.has_value()) {
      report_expected(scan, "partial-lookup in partial-apply");
    }
    ast::partial_lookup lookup =
        std::move(partial_lookup).consume_and_advance(&scan);

    if (!try_consume_token(&scan, tok::close)) {
      report_expected(
          scan,
          fmt::format(
              "{} to close partial-apply '{}'",
              tok::close,
              lookup.as_string()));
    }

    return {
        ast::partial_apply{
            scan.with_start(scan_start).range(),
            std::move(lookup),
            standalone_partial_offset(scan_start)},
        scan};
  }

  // partial-lookup → { path-component ~ ("/" ~ path-component)* }
  parse_result<ast::partial_lookup> parse_partial_lookup(
      parser_scan_window scan) {
    assert(scan.empty());
    const auto scan_start = scan.start;

    const token& first_component = scan.advance();
    if (first_component.kind != tok::path_component) {
      return no_parse_result();
    }
    std::vector<ast::path_component> path;
    path.emplace_back(ast::path_component{
        first_component.range, std::string(first_component.string_value())});

    while (try_consume_token(&scan, tok::slash)) {
      const token& component_part = scan.peek();
      if (component_part.kind != tok::path_component) {
        report_expected(scan, "path-component in partial-lookup");
      }
      scan.advance();
      path.emplace_back(ast::path_component{
          component_part.range, std::string(component_part.string_value())});
    }
    return {
        ast::partial_lookup{
            scan.with_start(scan_start).range(), std::move(path)},
        scan};
  }

 public:
  parser(
      std::vector<token> tokens,
      diagnostics_engine& diags,
      standalone_lines_scanner::result standalone_markings)
      : tokens_(std::move(tokens)),
        diags_(diags),
        standalone_markings_(std::move(standalone_markings)) {}

  std::optional<ast::root> parse() {
    return parse_root(parser_scan_window(
        tokens_.cbegin() /* start */,
        tokens_.cbegin() /* head */,
        tokens_.cend() /* end */));
  }
};

} // namespace

std::optional<ast::root> parse(source src, diagnostics_engine& diags) {
  auto tokens = lexer(std::move(src), diags).tokenize_all();
  auto standalone_scanner_result = standalone_lines_scanner::mark(tokens);
  return parser(std::move(tokens), diags, std::move(standalone_scanner_result))
      .parse();
}

} // namespace whisker

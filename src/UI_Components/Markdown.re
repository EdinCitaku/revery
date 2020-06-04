open Revery_Core;
open Revery_UI;
open Revery_UI_Primitives;
open Revery_Font;

module LinkComponent = Link;

open Omd;

module Log = (val Log.withNamespace("Revery.Components.Markdown"));

type style = {
  paragraph: list(Style.textStyleProps),
  activeLink: list(Style.textStyleProps),
  inactiveLink: list(Style.textStyleProps),
  inlineCode: list(Style.textStyleProps),
  h1: list(Style.textStyleProps),
  h2: list(Style.textStyleProps),
  h3: list(Style.textStyleProps),
  h4: list(Style.textStyleProps),
  h5: list(Style.textStyleProps),
  h6: list(Style.textStyleProps),
  fontFamily: Family.t,
  codeFontFamily: Family.t,
  baseFontSize: float,
  codeBlockBackgroundColor: Color.t,
};

module SyntaxHighlight = {
  type block = {
    byteIndex: int,
    color: Color.t,
    bold: bool,
    italicized: bool,
  }
  and t = (~language: string, list(string)) => list(list(block));

  let default: t =
    (~language as _, lines) => {
      let block = {
        byteIndex: 0,
        color: Colors.white,
        bold: false,
        italicized: false,
      };
      List.init(List.length(lines), _ => [block]);
    };
};

module Styles = {
  open Style;
  let inline = [
    flexDirection(`Row),
    flexWrap(`Wrap),
    alignItems(`FlexStart),
  ];
  let container = [justifyContent(`FlexStart), alignItems(`FlexStart)];
  let hardBreak = [height(6)];

  module Blockquote = {
    let container = [
      flexDirection(`Row),
      borderLeft(~width=4, ~color=Colors.grey),
    ];
    let contents = [
      paddingLeft(8),
      justifyContent(`FlexStart),
      alignItems(`FlexStart),
      flexGrow(1),
    ];
  };

  module Code = {
    let inlineContainer = [
      backgroundColor(Color.rgba(0., 0., 0., 0.25)),
      borderRadius(3.),
      border(~width=3, ~color=Color.rgba(0., 0., 0., 0.25)),
    ];

    let blockContainer = bgColor => [
      backgroundColor(bgColor),
      border(~width=3, ~color=bgColor),
      borderRadius(3.),
      padding(2),
      marginTop(4),
      marginBottom(4),
    ];

    let labelContainer = [
      flexDirection(`Row),
      alignItems(`FlexEnd),
      justifyContent(`FlexEnd),
    ];
  };

  module List = {
    let marker = [marginRight(6)];
    let contents = [
      justifyContent(`FlexStart),
      alignItems(`FlexStart),
      flexGrow(1),
    ];
  };

  module ThematicBreak = {
    let hr = [
      flexGrow(1),
      height(2),
      marginTop(2),
      marginBottom(2),
      backgroundColor(Color.rgba(0., 0., 0., 0.25)),
    ];
  };
};

type inlineAttrs =
  | Italicized
  | Bolded
  | Monospaced;

type kind = [ | `Paragraph | `Heading(int) | `Link(string) | `InlineCode];

let selectStyleFromKind = (kind: kind, styles) =>
  switch (kind) {
  | `Heading(1) => styles.h1
  | `Heading(2) => styles.h2
  | `Heading(3) => styles.h3
  | `Heading(4) => styles.h4
  | `Heading(5) => styles.h5
  | `Heading(6)
  | `Heading(_) => styles.h6
  | `InlineCode => styles.inlineCode
  | _ => styles.paragraph
  };

// Sourced from: http://zuga.net/articles/html-heading-elements/
let fontSizeFromKind = (kind: kind, styles) =>
  switch (kind) {
  | `Heading(1) => styles.baseFontSize *. 2.
  | `Heading(2) => styles.baseFontSize *. 1.5
  | `Heading(3) => styles.baseFontSize *. 1.17
  | `Heading(4) => styles.baseFontSize *. 1.
  | `Heading(5) => styles.baseFontSize *. 0.83
  | `Heading(6)
  | `Heading(_) => styles.baseFontSize *. 0.67
  | `InlineCode => styles.baseFontSize *. 0.75
  | _ => styles.baseFontSize
  };

type attrs = {
  inline: list(inlineAttrs),
  kind,
};

let isBold = attrs => List.mem(Bolded, attrs.inline);
let isItalicized = attrs => List.mem(Italicized, attrs.inline);
let isMonospaced = attrs => List.mem(Monospaced, attrs.inline);

let generateText = (text, styles, attrs) => {
  let fontSize = fontSizeFromKind(attrs.kind, styles);
  let fontWeight = {
    isBold(attrs) ? Weight.Bold : Weight.Normal;
  };

  switch (attrs.kind) {
  | `Link(href) =>
    <LinkComponent
      text
      activeStyle={styles.activeLink}
      inactiveStyle={styles.inactiveLink}
      fontSize
      fontFamily={styles.fontFamily}
      fontWeight
      italicized={isItalicized(attrs)}
      monospaced={isMonospaced(attrs)}
      href
    />
  | `InlineCode =>
    <View style=Styles.Code.inlineContainer>
      <Text
        text
        fontSize
        fontFamily={styles.fontFamily}
        fontWeight
        style={selectStyleFromKind(attrs.kind, styles)}
        italicized={isItalicized(attrs)}
        monospaced={isMonospaced(attrs)}
      />
    </View>
  | _ =>
    <Text
      text
      fontSize
      fontFamily={styles.fontFamily}
      fontWeight
      style={selectStyleFromKind(attrs.kind, styles)}
      italicized={isItalicized(attrs)}
      monospaced={isMonospaced(attrs)}
    />
  };
};

let generateCodeBlock =
    (codeBlock: Code_block.t, styles, highlighter: SyntaxHighlight.t) => {
  let label =
    switch (codeBlock.label) {
    | Some("")
    | None => None
    | Some(_) as label => label
    };
  Log.debugf(m =>
    m("Code block has label : %s", Option.value(label, ~default="(none)"))
  );

  let fontSize = fontSizeFromKind(`InlineCode, styles);

  <View style={Styles.Code.blockContainer(styles.codeBlockBackgroundColor)}>
    {switch (label, codeBlock.code) {
     | (None, Some(code)) =>
       <Text
         text=code
         fontFamily={styles.codeFontFamily}
         monospaced=true
         fontSize
         style={styles.paragraph}
       />
     | (Some(label), Some(code)) =>
       let lines = String.split_on_char('\n', code);
       let highlights: list(list(SyntaxHighlight.block)) =
         highlighter(~language=label, lines);
       List.map2(
         (line, highlight) => {
           <View style=Styles.inline>
             {List.mapi(
                (i, block: SyntaxHighlight.block) => {
                  let endIndex =
                    switch (List.nth_opt(highlight, i + 1)) {
                    | Some((blk: SyntaxHighlight.block)) => blk.byteIndex
                    | None => String.length(line)
                    };
                  let length = endIndex - block.byteIndex;
                  let text = String.sub(line, block.byteIndex, length);
                  <Text
                    text
                    style=Style.[color(block.color)]
                    fontFamily={styles.fontFamily}
                    fontWeight={block.bold ? Weight.Bold : Weight.Normal}
                    monospaced=true
                    fontSize
                  />;
                },
                highlight,
              )
              |> React.listToElement}
           </View>
         },
         lines,
         highlights,
       )
       |> React.listToElement;

     | (_, _) => <View />
     }}
  </View>;
};

let rec generateInline' = (inline, styles, attrs) => {
  switch (inline) {
  | Html(t)
  | Text(t) => generateText(t, styles, attrs)
  | Emph(e) =>
    generateInline'(
      e.content,
      styles,
      switch (e.style) {
      | Star => {...attrs, inline: [Bolded, ...attrs.inline]}
      | Underscore => {...attrs, inline: [Italicized, ...attrs.inline]}
      },
    )
  | Soft_break => generateText(" ", styles, attrs)
  | Hard_break => <View style=Styles.hardBreak />
  | Ref(r) =>
    generateInline'(
      r.label,
      styles,
      {...attrs, kind: `Link(r.def.destination)},
    )
  | Link(l) =>
    generateInline'(
      l.def.label,
      styles,
      {...attrs, kind: `Link(l.def.destination)},
    )
  | Code(c) =>
    generateText(
      c.content,
      styles,
      {kind: `InlineCode, inline: [Monospaced, ...attrs.inline]},
    )
  | Concat(c) =>
    <View style=Styles.inline>
      {c
       |> List.map(il => generateInline'(il, styles, attrs))
       |> React.listToElement}
    </View>
  | _ => <View />
  };
};

let generateInline = (inline, styles, attrs) =>
  <Row> {generateInline'(inline, styles, attrs)} </Row>;

let rec generateMarkdown' = (element, styles, highlighter) =>
  switch (element) {
  | Paragraph(p) => generateInline(p, styles, {inline: [], kind: `Paragraph})
  // We don't support HTML rendering as of right now, so we'll just render it
  // as text
  | Html_block(html) =>
    generateInline(Text(html), styles, {inline: [], kind: `Paragraph})
  | Blockquote(blocks) =>
    <View style=Styles.Blockquote.container>
      <View style=Styles.Blockquote.contents>
        {List.map(
           block => generateMarkdown'(block, styles, highlighter),
           blocks,
         )
         |> React.listToElement}
      </View>
    </View>
  | Heading(h) =>
    generateInline(
      h.text,
      styles,
      {inline: [Bolded], kind: `Heading(h.level)},
    )
  | Code_block(cb) => generateCodeBlock(cb, styles, highlighter)
  | List(blist) =>
    <View>
      {List.mapi(
         (i, blocks) => {
           let text =
             switch (blist.kind) {
             | Ordered(_, _) => string_of_int(i + 1) ++ "."
             | Unordered(_) => "•"
             };
           <View style=Styles.inline>
             <Text
               text
               style=Styles.List.marker
               fontFamily={styles.fontFamily}
             />
             <View style=Styles.List.contents>
               {List.map(
                  block => generateMarkdown'(block, styles, highlighter),
                  blocks,
                )
                |> React.listToElement}
             </View>
           </View>;
         },
         blist.blocks,
       )
       |> React.listToElement}
    </View>
  | Thematic_break =>
    <View style=Style.[flexDirection(`Row)]>
      <View style=Styles.ThematicBreak.hr />
    </View>
  | _ => <View />
  };

let generateMarkdown = (mdText: string, styles, highlighter) => {
  let md = Omd.of_string(mdText);
  Log.debugf(m => m("Parsed Markdown as: %s", Omd.to_sexp(md)));
  List.map(elt => generateMarkdown'(elt, styles, highlighter), md)
  |> React.listToElement;
};

let make =
    (
      ~markdown as mdText="",
      ~fontFamily=Family.default,
      ~codeFontFamily=Family.default,
      ~baseFontSize=14.0,
      ~paragraphStyle=Style.emptyTextStyle,
      ~activeLinkStyle=Style.emptyTextStyle,
      ~inactiveLinkStyle=Style.emptyTextStyle,
      ~h1Style=Style.emptyTextStyle,
      ~h2Style=Style.emptyTextStyle,
      ~h3Style=Style.emptyTextStyle,
      ~h4Style=Style.emptyTextStyle,
      ~h5Style=Style.emptyTextStyle,
      ~h6Style=Style.emptyTextStyle,
      ~inlineCodeStyle=Style.emptyTextStyle,
      ~codeBlockBackgroundColor=Color.rgba(0., 0., 0., 0.25),
      ~syntaxHighlighter=SyntaxHighlight.default,
      (),
    ) => {
  <View style=Styles.container>
    {generateMarkdown(
       mdText,
       {
         paragraph: paragraphStyle,
         activeLink: activeLinkStyle,
         inactiveLink: inactiveLinkStyle,
         inlineCode: inlineCodeStyle,
         h1: h1Style,
         h2: h2Style,
         h3: h3Style,
         h4: h4Style,
         h5: h5Style,
         h6: h6Style,
         fontFamily,
         codeFontFamily,
         baseFontSize,
         codeBlockBackgroundColor,
       },
       syntaxHighlighter,
     )}
  </View>;
};
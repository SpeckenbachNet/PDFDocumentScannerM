#!/usr/bin/env python3
"""Turns the help of the app (help/<de|en>/*.md) into the documentation pages of the website,
in the same HTML as the MovieMetaM documentation (Joomla article, Bootstrap classes, mm-* styles).

Call:    tools/help-to-html.py <help folder> <output folder>
Result:  <output folder>/documentation-en.html and documentation-de.html

Only the Markdown the help uses is understood: # and ## headings, paragraphs (two spaces at the
end of a line: line break), lists with - and 1., > notes, images in a line of their own,
**bold**, *italic*, `code` and [links](url). Bold text that is the title of another topic becomes
a link to it (not inside that topic itself). Called by tools/build-linux.sh; it can also run on its own.
"""

import html
import re
import sys
from pathlib import Path

APP = "PDFDocumentScannerM"
SLUG = "pdfdocumentscannerm"
IMAGE_DIR = f"images/software/{SLUG}/help/"

# Order of the help page in the app (HelpPage, enum Topic)
TOPICS = ["scanning", "pages", "settings", "saving", "about"]

TEXTS = {
    "en": {"title": f"{APP} – Documentation", "contents": "Contents", "about": f"About {APP}",
           "other": "de", "other_name": "Deutsch"},
    "de": {"title": f"{APP} – Dokumentation", "contents": "Inhalt", "about": f"Über {APP}",
           "other": "en", "other_name": "English"},
}


def inline(text, titles):
    """Inline Markdown of one block; titles: topic title -> section id"""
    text = html.escape(text, quote=False)

    def bold(match):
        inner = match.group(1)
        if inner in titles:
            return f'<a href="#{titles[inner]}"><strong>{inner}</strong></a>'
        return f"<strong>{inner}</strong>"

    text = re.sub(r"`([^`]+)`", r"<code>\1</code>", text)
    text = re.sub(r"\*\*(.+?)\*\*", bold, text)
    text = re.sub(r"(?<!\*)\*([^*\s][^*]*?)\*(?!\*)", r"<em>\1</em>", text)
    text = re.sub(r"\[([^\]]+)\]\(([^)\s]+)\)", r'<a href="\2">\1</a>', text)
    return text


def figure(alt, src):
    if src.startswith(":/images/help/"):
        src = IMAGE_DIR + src[len(":/images/help/"):]
    alt = html.escape(alt)
    return ('<figure class="figure d-block text-center my-3"><img class="figure-img img-fluid '
            f'rounded border" src="{src}" alt="{alt}" loading="lazy"><figcaption '
            f'class="figure-caption">{alt}</figcaption></figure>')


def paragraph(lines, titles):
    """Lines of one paragraph; two spaces at the end of a line give a line break"""
    parts = []
    for i, line in enumerate(lines):
        last = i == len(lines) - 1
        text = inline(line.strip(), titles)
        parts.append(text if last or not line.endswith("  ") else text + "<br>")
    joined = ""
    for i, part in enumerate(parts):
        if i:
            joined += "\n" if parts[i - 1].endswith("<br>") else " "
        joined += part
    return joined


def convert(lines, titles):
    """Body of one topic (without its # heading)"""
    out = []
    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        if not stripped:
            i += 1
            continue
        if stripped.startswith("## "):
            out.append(f'<h3 class="h5 mt-4">{inline(stripped[3:], titles)}</h3>')
            i += 1
            continue
        image = re.fullmatch(r"!\[([^\]]*)\]\(([^)\s]+)\)", stripped)
        if image:
            out.append(figure(image.group(1), image.group(2)))
            i += 1
            continue
        if stripped.startswith(">"):
            note = []
            while i < len(lines) and lines[i].strip().startswith(">"):
                note.append(lines[i].strip()[1:].lstrip())
                i += 1
            out.append(f'<div class="mm-story my-3 ms-0"><p class="mb-0">{paragraph(note, titles)}</p></div>')
            continue
        list_kind = "ul" if re.match(r"[-*] ", stripped) else "ol" if re.match(r"\d+\. ", stripped) else None
        if list_kind:
            marker = r"[-*] " if list_kind == "ul" else r"\d+\. "
            items = []
            while i < len(lines) and re.match(marker, lines[i].strip()):
                item = [re.sub(r"^" + marker, "", lines[i].strip())]
                i += 1
                # Continuation lines of an item are indented
                while i < len(lines) and lines[i].startswith(("  ", "\t")) and lines[i].strip() \
                        and not re.match(r"([-*]|\d+\.) ", lines[i].strip()):
                    item.append(lines[i].strip())
                    i += 1
                items.append(f"<li>{paragraph(item, titles)}</li>")
            out.append(f"<{list_kind}>\n" + "\n".join(items) + f"\n</{list_kind}>")
            continue
        block = []
        while i < len(lines) and lines[i].strip() and not lines[i].strip().startswith(("#", ">", "![")) \
                and not re.match(r"([-*]|\d+\.) ", lines[i].strip()):
            block.append(lines[i].rstrip("\n"))
            i += 1
        out.append(f"<p>{paragraph(block, titles)}</p>")
    return out


def read_topic(folder, topic, texts):
    """(title, body lines) of one help file; the about page gets the title of the help list"""
    lines = (folder / f"{topic}.md").read_text(encoding="utf-8").splitlines()
    title = topic
    if lines and lines[0].startswith("# "):
        title = lines[0][2:].strip()
        lines = lines[1:]
    if topic == "about":
        title = texts["about"]
        # The version line is filled in by the app ({{version}}); the website shows its own
        lines = [line for line in lines if "{{version}}" not in line]
    return title, lines


def page(lang, help_dir):
    texts = TEXTS[lang]
    folder = help_dir / lang
    topics = [(topic, *read_topic(folder, topic, texts)) for topic in TOPICS]
    titles = {title: topic for topic, title, _ in topics}

    other = texts["other"]
    out = ['<div class="mm-page">',
           '<div class="d-flex flex-wrap justify-content-between align-items-center gap-2 mb-4">',
           f'    <h1 class="mb-0">{texts["title"]}</h1>',
           f'    <a class="btn btn-outline-primary btn-sm" href="{other}/{SLUG}/documentation" '
           f'hreflang="{other}">{texts["other_name"]}</a>',
           '</div>',
           f'<nav class="card mb-5" aria-label="{texts["contents"]}">',
           '  <div class="card-body">',
           f'    <h2 class="h5 card-title">{texts["contents"]}</h2>',
           '    <ol class="mb-0" style="columns: 2; max-width: 30rem">'
           + "".join(f'<li><a href="#{topic}">{html.escape(title)}</a></li>' for topic, title, _ in topics)
           + '</ol>',
           '  </div>',
           '</nav>']
    for topic, title, lines in topics:
        out += ["", f'<section class="mb-5" id="{topic}">',
                f'<h2 class="border-bottom pb-2 mb-3">{html.escape(title)}</h2>']
        # No link to the section itself (German: the button "Scannen" is also the topic's title)
        out += convert(lines, {title: target for title, target in titles.items() if target != topic})
        out.append("</section>")
    out.append("</div>")
    return "\n".join(out) + "\n"


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    help_dir, out_dir = Path(sys.argv[1]), Path(sys.argv[2])
    out_dir.mkdir(parents=True, exist_ok=True)
    for lang in TEXTS:
        target = out_dir / f"documentation-{lang}.html"
        target.write_text(page(lang, help_dir), encoding="utf-8")
        print(target)


if __name__ == "__main__":
    main()

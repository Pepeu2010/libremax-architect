"""Read official public pages only; never download proprietary product assets."""
import html
import re
import urllib.request
from html.parser import HTMLParser

class VisibleText(HTMLParser):
    def __init__(self):
        super().__init__()
        self.skip = 0
        self.parts = []
    def handle_starttag(self, tag, attrs):
        if tag in ("script", "style"):
            self.skip += 1
    def handle_endtag(self, tag):
        if tag in ("script", "style"):
            self.skip = max(0, self.skip - 1)
    def handle_data(self, data):
        if not self.skip and data.strip():
            self.parts.append(data.strip())

for url in ("https://www.vdmax.com/vdmaxad", "https://www.vdmax.com/tabela-comparativa-vdmax", "https://www.vdmax.com/video-tips", "https://www.vdmax.com/suporte"):
    try:
        with urllib.request.urlopen(url, timeout=30) as response:
            page = response.read(8_000_000).decode("utf-8")
        links = sorted(set(html.unescape(x) for x in re.findall(r'https?[^\s"<>]+', page)))
        print(url)
        parser = VisibleText()
        parser.feed(page)
        print("\n".join(parser.parts)[-11000:])
        for link in links:
            if any(key in link.lower() for key in ("youtube.com/embed", "help", "zendesk")):
                print(link[:500])
    except Exception as error:
        print(url, type(error).__name__, str(error))

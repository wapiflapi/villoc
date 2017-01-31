#!/usr/bin/env python3
# @wapiflapi

import re
import html
import random
import codecs


class State(list):

    def __init__(self, *args, **kwargs):
        self.errors = []
        self.info = []
        super().__init__(*args, **kwargs)

    def boundaries(self):
        bounds = set()
        for block in self:
            lo, hi = block.boundaries()
            bounds.add(lo)
            bounds.add(hi)
        return bounds



class Printable():

    unit_width = 10
    classes = ["block"]

    def boundaries(self):
        return (self.start(), self.end())

    def gen_html(self, out, width, color=""):
        out.write('<div class="%s" style="width: %dem; %s;">' %
                  (" ".join(self.classes), 10 * width, color))
        if self.details:
            out.write('<strong>%#x</strong><br />' % self.start())
            out.write(self.more_html())
        else:
            out.write('&nbsp;')

        out.write('</div>\n')

    def more_html(self):
        return ""

    def __repr__(self):
        return "%s(start=%#x, end=%#x)" % (self.__class__.__name__,
                                           self.start(), self.end())


class Empty(Printable):

    classes = Printable.classes + ["empty"]

    def __init__(self, start, end, display=True):
        self._start = start
        self._end = end
        self.details = display

    def start(self):
        return self._start

    def end(self):
        return self._end

    def set_end(self, end):
        self._end = end

    def more_html(self):
        return "+ %#x" % (self.end() - self.start())


class Block(Printable):

    header = 8
    footer = 0
    round = 0x10
    minsz = 0x20

    classes = Printable.classes + ["normal"]

    def __init__(self, addr, size, error=False, tmp=False, **kwargs):
        self.color = kwargs.get('color', random_color())
        self.uaddr = addr
        self.usize = size
        self.details = True
        self.error = error
        self.tmp = tmp

    def start(self):
        return self.uaddr - self.header

    def end(self):
        size = max(self.minsz, self.usize + self.header + self.footer)
        rsize = size + (self.round - 1)
        rsize = rsize - (rsize % self.round)
        return self.uaddr - self.header + rsize

    def gen_html(self, out, width):

        if self.color:
            color = ("background-color: rgb(%d, %d, %d);" % self.color)
        else:
            color = ""

        if self.error:
            color += ("background-image: repeating-linear-gradient("
                      "120deg, transparent, transparent 1.40em, "
                      "#A85860 1.40em, #A85860 2.80em);")

        super().gen_html(out, width, color)

    def more_html(self):
        return "+ %#x (%#x)" % (self.end() - self.start(), self.usize)

    def __repr__(self):
        return "%s(start=%#x, end=%#x, tmp=%s)" % (
            self.__class__.__name__, self.start(), self.end(), self.tmp)


class Marker(Block):

    def __init__(self, addr, error=False, **kwargs):
        super().__init__(addr, 0x0, tmp=True, error=error, *kwargs)

    def more_html(self):
        return "unknown"


def match_ptr(state, ptr):

    if ptr is None:
        return None, None

    s, smallest_match = None, None

    for i, block in enumerate(state):
        if block.uaddr != ptr:
            continue
        if smallest_match is None or smallest_match.usize >= block.usize:
            s, smallest_match = i, block

    if smallest_match is None:
        state.errors.append("Couldn't find block at %#x, added marker." %
                            (ptr - Block.header))
        # We'll add a small tmp block here to show the error.
        state.append(Marker(ptr, error=True))

    return s, smallest_match


def malloc(state, ret, size):

    if not ret:
        state.errors.append("Failed to allocate %#x bytes." % size)
    else:
        state.append(Block(ret, size))


def calloc(state, ret, nmemb, size):
    malloc(state, ret, nmemb * size)


def free(state, ret, ptr):

    if ptr is 0:
        return

    s, match = match_ptr(state, ptr)

    if match is None:
        return
    elif ret is None:
        state[s] = Block(match.uaddr, match.usize,
                         error=True, color=match.color)
    else:
        del state[s]


def realloc(state, ret, ptr, size):

    if not ptr:
        return malloc(state, ret, size)
    elif not size:
        return free(state, ret, ptr)

    s, match = match_ptr(state, ptr)

    if match is None:
        return
    elif ret is None:
        state[s] = Block(match.uaddr, match.usize, color=match.color)
        state[s].error = True
    else:
        state[s] = Block(ret, size, color=match.color)


def meta(state, ret, msg):
    return ([], ["after: %s" % (msg,)])

operations = {
    'free': free,
    'malloc': malloc,
    'calloc': calloc,
    'realloc': realloc,
    'villoc': meta,
}


def sanitize(x):
    if x is None:
        return None
    if x == "<void>":
        return 0
    if x == "(nil)":
        return 0
    try:
        return int(x, 0)
    except:
        return x


def parse_ltrace(ltrace):

    match_call = r"^([a-z_]+)\((.*)\) += (.*)"
    match_err = r"^([a-z_]+)\((.*) <no return \.\.\.>"

    for line in ltrace:

        # if the trace file contains PID (for ltrace -f)
        head, _, tail = line.partition(" ")
        if head.isdigit():
            line = tail

        if not any(line.startswith(f) for f in operations):
            continue

        try:
            func, args, ret = re.findall(match_call, line)[0]
        except Exception:

            try:
                # maybe this stopped the program
                func, args = re.findall(match_err, line)[0]
                ret = None
            except Exception:
                print("ignoring line: %s" % line, file=sys.stderr)
                continue

        print("%s" % (line.strip(),), file=sys.stderr)
        args = list(map(sanitize, args.split(", ")))
        ret = sanitize(ret)

        yield func, args, ret


def build_timeline(events):

    boundaries = set()
    timeline = [State()]
    errors = []
    info = []

    for func, args, ret in events:

        try:
            op = operations[func]
        except KeyError:
            continue

        state = State(b for b in timeline[-1] if not b.tmp)

        meta = op(state, ret, *args)
        if meta:
            errors.extend(meta[0])
            info.extend(meta[1])
            continue
        else:
            state.errors.extend(errors)
            state.info.extend(info)
            errors = []
            info = []

        call = "%s(%s)" % (func, ", ".join("%#x" % a for a in args))

        if ret is None:
            state.errors.append("%s = <error>" % call)
        else:
            state.info.append("%s = %#x" % (call, ret))

        boundaries.update(state.boundaries())
        timeline.append(state)

    return timeline, boundaries


def random_color(r=200, g=200, b=125):

    red = (random.randrange(0, 256) + r) / 2
    green = (random.randrange(0, 256) + g) / 2
    blue = (random.randrange(0, 256) + b) / 2

    return (red, green, blue)


def print_state(out, boundaries, state):

    out.write('<div class="state %s">\n' % ("error" if state.errors else ""))

    known_stops = set()

    todo = state
    while todo:

        out.write('<div class="line" style="">\n')

        done = []

        current = None
        last = 0

        for i, b in enumerate(boundaries):

            # If this block has size 0; make it continue until the
            # next boundary anyway. The size will be displayed as
            # 0 or unknown anyway and it shouldn't be too confusing.
            if current and current.end() != b and current.start() != current.end():
                continue

            if current:  # stops here.
                known_stops.add(i)
                current.gen_html(out, i - last)
                done.append(current)
                last = i

            current = None
            for block in todo:
                if block.start() == b:
                    current = block
                    break
            else:
                continue

            if last != i:

                # We want to show from previous known_stop.

                for s in reversed(range(last, i+1)):
                    if s in known_stops:
                        break

                if s != last:
                    Empty(boundaries[last], boundaries[s],
                          display=False).gen_html(out, s - last)
                    known_stops.add(s)

                if s != i:
                    Empty(boundaries[s], b).gen_html(out, i - s)
                    known_stops.add(i)

                last = i


        if current:
            raise RuntimeError("Block was started but never finished.")

        if not done:
            raise RuntimeError("Some block(s) don't match boundaries.")

        out.write('</div>\n')

        todo = [x for x in todo if x not in done]

    out.write('<div class="log">')

    for msg in state.info:
        out.write('<p>%s</p>' % html.escape(str(msg)))

    for msg in state.errors:
        out.write('<p>%s</p>' % html.escape(str(msg)))

    out.write('</div>\n')

    out.write('</div>\n')


def gen_html(timeline, boundaries, out):

    if timeline and not timeline[0]:
        timeline.pop(0)

    boundaries = list(sorted(boundaries))

    out.write('<style>')

    out.write('''body {
font-size: 12px;
background-color: #EBEBEB;
font-family: "Lucida Console", Monaco, monospace;
width: %dem;
}
''' % ((len(boundaries) - 1) * (Printable.unit_width + 1)))

    out.write('''p {
margin: 0.8em 0 0 0.1em;
}
''')

    out.write('''.block {
float: left;
padding: 0.5em 0;
text-align: center;
color: black;
}
''')

    out.write('''.normal {

-webkit-box-shadow: 2px 2px 4px 0px rgba(0,0,0,0.80);
-moz-box-shadow: 2px 2px 4px 0px rgba(0,0,0,0.80);
box-shadow: 2px 2px 4px 0px rgba(0,0,0,0.80);

}
''')

    out.write('''.empty + .empty {
border-left: 1px solid gray;
margin-left: -1px;
}
''')

    out.write('''.empty {
color: gray;
}
''')

    out.write('.line {  }')

    out.write('''.line:after {
  content:"";
  display:table;
  clear:both;
}''')

    out.write('''.state {
margin: 0.5em; padding: 0;

background-color: white;

border-radius: 0.3em;

-webkit-box-shadow: inset 2px 2px 4px 0px rgba(0,0,0,0.80);
-moz-box-shadow: inset 2px 2px 4px 0px rgba(0,0,0,0.80);
box-shadow: inset 2px 2px 4px 0px rgba(0,0,0,0.80);

padding: 0.5em;

}''')

    out.write('''.log {
}''')

    out.write('''.error {
color: white;
background-color: #8b1820;
}''')

    out.write('''.error .empty {
color: white;
}''')

    out.write('</style>\n')

    out.write('<script src="https://ajax.googleapis.com/ajax/libs/jquery/2.1.3/jquery.min.js"></script>')
    out.write('''<script>
var scrollTimeout = null;
$(window).scroll(function(){
    if (scrollTimeout) clearTimeout(scrollTimeout);
    scrollTimeout = setTimeout(function(){
    $('.log').stop();
    $('.log').animate({
        'margin-left': $(this).scrollLeft()
    }, 100);
    }, 200);
});
</script>
''')

    out.write('<body>\n')

    out.write('<div class="timeline">\n')

    for state in timeline:
        print_state(out, boundaries, state)

    out.write('</div>\n')

    out.write('</body>\n')


if __name__ == '__main__':

    import sys
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("ltrace", type=argparse.FileType("rb"))
    parser.add_argument("out", type=argparse.FileType("w"))
    parser.add_argument("--header", type=int, default=8,
                        help="size of malloc metadata before user data")
    parser.add_argument("--footer", type=int, default=0,
                        help="size of malloc metadata after user data")
    parser.add_argument("--round", type=int, default=0x10,
                        help="size of malloc chunks are a multiple of this value")
    parser.add_argument("--minsz", type=int, default=0x20,
                        help="size of a malloc chunk is at least this value")
    parser.add_argument("--raw", action="store_true",
                        help="disables header, footer, round and minsz")

    # Some values that work well: 38, 917, 190, 226
    parser.add_argument("-s", "--seed", type=int, default=226)
    parser.add_argument("-S", "--show-seed", action="store_true")
    args = parser.parse_args()

    random.seed(args.seed)

    if args.show_seed:
        args.out.write('<h2>seed: %d</h2>' % args.seed)

    # Set malloc options

    if args.raw:
        Block.header, Block.footer, Block.round, Block.minsz = 0, 0, 1, 0
    Block.header, Block.footer, Block.round, Block.minsz = (
        args.header, args.footer, args.round, args.minsz)


    noerrors = codecs.getreader('utf8')(args.ltrace.detach(), errors='ignore')
    timeline, boundaries = build_timeline(parse_ltrace(noerrors))

    gen_html(timeline, boundaries, args.out)

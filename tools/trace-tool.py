import argparse
import struct
import sqlite3

class TraceEventOpcode:
    Edge = 0
    Read = 1
    Write = 2


class TraceEdgeType:
    Branch = 0
    Call = 1


class MgbaTrace:
    def __init__(self, basic_blocks, functions, edges, event_count):
        self.basic_blocks = basic_blocks
        self.functions = functions
        self.edges = edges
        self.event_count = event_count


def load_trace(trace_path, verbose):
    fp = open(trace_path, 'rb')
    bbs = {}  # addr -> count mapping
    functions = set()  # set of function addresses
    edges = set()  # set of (src, dst, type)

    event_count = 0

    while True:
        opcode = fp.read(1)

        # Reached eof
        if len(opcode) == 0:
            break

        opcode = struct.unpack("B", opcode)[0]

        if opcode == TraceEventOpcode.Edge:
            event_data = fp.read(9)

            if len(event_data) != 9:
                raise Exception('Unexpected eof while reading edge entry')

            source, target, edge_type = struct.unpack("<IIB", event_data)
            source -= 4  # pc offset

            if edge_type == TraceEdgeType.Call:
                functions.add(target)

            if (source, target, edge_type) not in edges:
                edges.add((source, target, edge_type))

            if target not in bbs:
                bbs[target] = 1
            else:
                bbs[target] += 1
        else:
            raise Exception(f"Unsupported opcode: 0x{opcode:x}")

        event_count += 1

    if verbose:
        print(f"Event count         : {event_count}")
        print(f"Unique edges        : {len(edges)}")
        print(f"Unique basic blocks : {len(bbs)}")
        print(f"Functions called    : {len(functions)}")

    return MgbaTrace(bbs, functions, edges, event_count)


def trace_to_db(trace_path, db_path, verbose):
    """ mgba trace -> trace database """
    trace = load_trace(trace_path, verbose)

    # Create and fill the database
    s = sqlite3.connect(db_path)
    c = s.cursor()

    c.execute("CREATE TABLE edges (source INTEGER NOT NULL, target INTEGER NOT NULL, type INTEGER NOT NULL)")
    c.execute("CREATE INDEX xrefs_to ON edges(target)")
    c.execute("CREATE INDEX xrefs_from ON edges(source)")
    c.execute("CREATE TABLE basic_blocks (address INTEGER NOT NULL PRIMARY KEY, hitcount INTEGER NOT NULL)")
    c.execute("CREATE TABLE functions (address INTEGER NOT NULL PRIMARY KEY)")

    for function in functions:
        c.execute("INSERT INTO functions VALUES (?)", (function,))

    for bb, hitcount in bbs.items():
        c.execute("INSERT INTO basic_blocks VALUES (?, ?)", (bb, hitcount))

    for edge in edges:
        c.execute("INSERT INTO edges VALUES (?, ?, ?)", edge)

    s.commit()
    s.close()


def trace_to_wakare_db(trace_path, db_path, verbose):
    """ mgba trace -> wakare compatible database """
    trace = load_trace(trace_path, verbose)
    s = sqlite3.connect(db_path)
    c = s.cursor()

    c.execute("CREATE TABLE branches (step INTEGER NOT NULL PRIMARY KEY, type INTEGER, source INTEGER, destination INTEGER);")
    c.execute("CREATE TABLE mappings (id INTEGER NOT NULL PRIMARY KEY, filename TEXT, start INTEGER, end INTEGER);")
    c.execute("CREATE TABLE hitcounts (id INTEGER NOT NULL PRIMARY KEY, address INTEGER, hitcount INTEGER);")

    # Insert gba mappings (needed for wakare to work)
    c.execute("INSERT INTO mappings (id, filename, start, end) VALUES (?, ?, ?, ?);",
              (0, "ROM", 0x08000000, 0x0a000000))

    for i, bb in enumerate(trace.basic_blocks.items()):
        # Sometimes there are a few weird artifacts in the trace (bios calls maybe ?)
        if bb[0] >= 0x08000000:
            c.execute("INSERT INTO hitcounts (id, address, hitcount) VALUES (?, ?, ?)",
                    (i, bb[0], bb[1]))

    s.commit()
    s.close()


def main():
    parser = argparse.ArgumentParser(description="mGBA trace to database converter")
    parser.add_argument("-o", "--output", help="Output database file", default="output.db")
    parser.add_argument("-v", "--verbose", help="Print stats about the trace", action="store_true")
    parser.add_argument("-w", "--wakare", help="Output a wakare compatible database", action="store_true")
    parser.add_argument("input", help="Input binary trace file")

    args = parser.parse_args()

    if not args.wakare:
        trace_to_db(args.input, args.output, args.verbose)
    else:
        trace_to_wakare_db(args.input, args.output, args.verbose)

if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""
Run a program and compare its output to what is expected.
"""

import argparse              # argparse
import difflib               # difflib.unified_diff
import os                    # os.getenv
import re                    # re.compile
import signal                # signal.signal
import subprocess            # subprocess.run
import sys                   # sys.argv, sys.stderr
import time                  # time.sleep
import traceback             # traceback.print_exc


# -------------- BEGIN: boilerplate -------------
# These are things I add at the start of every Python program to
# allow better error reporting.

# Positive if debug is enabled, with higher values enabling more printing.
debugLevel = 0
if (os.getenv("DEBUG")):
  debugLevel = int(os.getenv("DEBUG"))

def debugPrint(str):
  """Debug printout when DEBUG >= 2."""
  if debugLevel >= 2:
    print(str)

# Ctrl-C: interrupt the interpreter instead of raising an exception.
signal.signal(signal.SIGINT, signal.SIG_DFL)

class Error(Exception):
  """A condition to be treated as an error."""
  pass

def die(message):
  """Throw a fatal Error with message."""
  raise Error(message)

def exceptionMessage(e):
  """Turn exception 'e' into a human-readable message."""
  t = type(e).__name__
  s = str(e)
  if s:
    return f"{t}: {s}"
  else:
    return f"{t}"

def call_main():
  """Call main() and catch exceptions."""
  try:
    main()

  except SystemExit as e:
    raise      # Let this one go, otherwise sys.exit gets "caught".

  except BaseException as e:
    print(f"{exceptionMessage(e)}", file=sys.stderr)
    if (debugLevel >= 1):
      traceback.print_exc(file=sys.stderr)
    sys.exit(2)
# --------------- END: boilerplate --------------

def splitLines(data):
  """Decode byte list 'data' to text, split it at line boundaries
  (tolerating both LF and CRLF), and return the list of lines.  If the
  last line does not end with a newline, add another line indicating
  that fact."""

  text = data.decode()
  lines = text.splitlines()
  if len(text) > 0 and text[len(text)-1] != "\n":
    lines += ["\\ no newline"]
  return lines


def writeLinesToFile(lines, fname):
  """Write 'lines', which has no newlines, to 'fname' with LF line endings."""

  with open(fname, "w", newline="\n") as f:
    for line in lines:
      print(line, file=f)


# Hexadecimal numbers with at least two digits.
hexDigitsRE = re.compile(r"0x[0-9a-fA-F][0-9a-fA-F]+")

# Particular hex sequences that should not be replaced.
nonReplaceHexRE = re.compile(r"0x7F+");

def hexReplacer(m):
  """What to replace a match of 'hexDigitsRE' with."""

  if nonReplaceHexRE.match(m.group(0)):
    return m.group(0)
  else:
    return "0xHEXDIGITS"

def normalizeOutput(line):
  """Remove strings that vary from run to run so the result is suitable
  as expected test output."""

  line = hexDigitsRE.sub(hexReplacer, line)
  return line


def main():
  # Parse command line.
  parser = argparse.ArgumentParser()
  parser.add_argument("--actual",
    help="If provided, write the actual output to this file.")
  parser.add_argument("--expect", required=True,
    help="Expected output.")
  parser.add_argument("program",
    help="Program to run.")
  parser.add_argument("progArgs", nargs=argparse.REMAINDER,
    help="Arguments to the program.")
  opts = parser.parse_args()

  # Read the expected output.
  with open(opts.expect) as f:
    expectLines = [line.rstrip("\n") for line in f.readlines()]

  # Run the program, capturing output.
  proc = subprocess.run([opts.program]+opts.progArgs, capture_output=True);

  # Combine the stdout, stderr, and exit code into one list.
  actualLines = []
  actualLines += ["---- stdout ----"]
  actualLines += splitLines(proc.stdout)
  actualLines += ["---- stderr ----"]
  actualLines += splitLines(proc.stderr)
  actualLines += ["---- exit status ----"]
  actualLines += [f"Exit {proc.returncode}"]

  # Normalize it.
  actualLines = [normalizeOutput(line) for line in actualLines]

  # Optionally save it.
  if opts.actual:
    writeLinesToFile(actualLines, opts.actual)

  diff = list(difflib.unified_diff(expectLines, actualLines,
    fromfile="expect", tofile="actual", lineterm=""))
  if len(diff) > 0:
    print("Results are different:")
    for diffLine in diff:
      print(diffLine)

    if os.getenv("UPDATE_EXPECT"):
      print(f"UPDATE_EXPECT is set.  Updating {opts.expect} with the new output.")
      writeLinesToFile(actualLines, opts.expect)

    else:
      print("Re-run with UPDATE_EXPECT=1 to update.")
      sys.exit(2)


call_main()


# EOF

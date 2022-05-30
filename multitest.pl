#!/usr/bin/perl -w
# run a program on an input file, then (if the input has some
# stylized comments) re-run, expecting an error

use strict 'subs';
use Config;

$comment = "//";    # comment syntax
$selectedError = "";
$keepTemps = 0;
$contin = 0;
$curSUT = "";
@failures = ();     # if $contin, track which configurations failed

$me = "multitest";

while (@ARGV && $ARGV[0] =~ m/^-/) {
  my $opt = $ARGV[0];
  shift @ARGV;

  if ($opt eq "-err") {
    $selectedError = $ARGV[0];
    shift @ARGV;
    next;
  }

  if ($opt eq "-sut") {
    $curSUT = $ARGV[0];
    shift @ARGV;
    next;
  }

  if ($opt eq "-keep") {
    $keepTemps = 1;
    next;
  }

  if ($opt eq "-contin") {
    $contin = 1;
    next;
  }

  die("$me: unknown argument: $opt\n");
}

if (@ARGV < 2) {
  print(<<"EOF");
usage: $0 [options] program [args...] input.cc

This will first invoke the command line as given, expecting that to
succeed.  Then, it will scan input.cc (which must be the last argument
on the command line) for any lines of the forms:

  ${comment}ERROR(name): <some code>
  <some code>     ${comment}ERRORIFMISSING(name):

If it finds them, then for each such 'name' the lines ERROR(name) will
be uncommented (and "ERROR(name)" removed), and lines
ERRORIFMISSING(name) commented-out, and the original command executed
again.  These additional runs should fail.

Each 'name' must match the regex: [a-zA-Z0-9_-]+

After any ERROR/IFMISSING line, before the next one, you can write:

  ${comment}NOTWORKING(sut): explanation

to indicate that the preceding test does not work with the given
System Under Test (SUT).  When this script is invoked with the matching
-sut option, it will run the test but expect no failure.  The default
SUT is "".

options:
  -err name     Only test ERROR(name) (does not test original).
  -sut sut      Set current SUT for NOTWORKING lines.
  -keep         Keep temporaries even when they succeed.
EOF

  exit(0);
}


# excerpt from perlipc man page
defined $Config{sig_name} || die "No sigs?";
$i = 0;
foreach $name (split(' ', $Config{sig_name})) {
  $signo{$name} = $i;
  $signame[$i] = $name;
  $i++;
}

$sigint = $signo{INT};
$sigquit = $signo{QUIT};
#print("sigint: $sigint\n");


$fname = $ARGV[@ARGV - 1];
#print("fname: $fname\n");

($fnameBase, $fnameExt) = ($fname =~ m|^(.*)(\.[^./]*)$|);
                                   #    bse ext
if (!defined($fnameExt)) {
  $fnameBase = $fname;
  $fnameExt = "";     # no '.' present anywhere (after last '/')
}

# try once with no modifications
if (!$selectedError) {
  # I do not print any header line here because I invoke this script
  # a very large number of times as part of 'regrtest', and I do not
  # want the clutter of repeated banners there.

  $code = mysystem(@ARGV);
  if ($code != 0) {
    failed("original", $code);
  }
}

# bail, or if $contin, just keep track
sub failed {
  my ($config, $code) = @_;

  if ($contin) {
    push @failures, ($config);
  }
  else {
    exit($code);
  }
}


# read the input file
open(IN, "<$fname") or die("can't open $fname: $!\n");
@lines = <IN>;
close(IN) or die;

# Set of codes marked as not working with the current SUT.
%notworking = ();

# Set of ERROR/ERRORIFMISSING codes that are present.
%codes = ();

# Scan the file to populate %notworking and %codes.
$lastCode = "";
foreach $line (@lines) {
  my ($miss, $code) = ($line =~ m|${comment}ERROR(IFMISSING)?\(([a-zA-Z0-9_-]+)\):|);
  if (defined($code)) {
    $codes{$code} = 1;
    $miss .= " ";     # pretend used
    $lastCode = $code;
  }

  my ($sut) = ($line =~ m|${comment}NOTWORKING\(([a-zA-Z0-9_-]*)\):|);
  if (defined($sut) && $sut eq $curSUT) {
    $notworking{$lastCode} = 1;
  }
}

# get sorted keys
@allkeys = (sort (keys %codes));
$numkeys = @allkeys;
if ($numkeys == 0) {
  # no error tags
  exit(0);
}

# consider each in turn
$testedVariations = 0;
foreach $selcode (@allkeys) {
  if ($selectedError &&
      $selectedError ne $selcode) {
    next;
  }
  $testedVariations++;

  my $label = $notworking{$selcode}? "NOTWORKING" : "ERROR";
  print("-- selecting $label($selcode) --\n");

  # Include $sut in the generated file name so we can run tests for
  # other SUTs in parallel.
  my $sutPrefix = $curSUT? "${curSUT}-" : "";

  my $tempfname = "${fnameBase}.error.${sutPrefix}${selcode}${fnameExt}";

  # run through the lines in the file, generating a new file
  # that has the selected lines uncommented
  open(OUT, ">$tempfname") or die("can't create $tempfname: $!\n");
  foreach $line (@lines) {
    my ($miss, $code, $rest) =
      #                          miss          code               rest
      ($line =~ m|${comment}ERROR(IFMISSING)?\(([a-zA-Z0-9_-]+)\):(.*)$|);
    if (defined($code) && $selcode eq $code) {
      if ($miss) {
        # ERRORIFMISSING: we want to comment the whole line
        print OUT ("${comment} $line");
      }
      else{
        # ERROR: we want to uncomment what follows the "ERROR" marker
        print OUT ($rest, "\n");
      }
    }
    elsif ($line =~ m|collectLookupResults|) {
      # comment-out this line in the error cases because if I do not
      # then it will often lead to its own error, which would mask the
      # one I am trying to verify
      print OUT ("${comment} $line");
    }
    else {
      print OUT ($line);         # emit as-is
    }
  }
  close(OUT) or die;

  # run the command on the new input file
  @args = @ARGV;
  $args[@ARGV - 1] = $tempfname;

  #print("command: ", join(' ', @args), "\n");
  $code = mysystem(@args);

  if ($notworking{$selcode}) {
    if ($code == 0) {
      print("$selcode: No error, as expected.\n");
      if (!$keepTemps) {
        unlink($tempfname);
      }
    }
    else {
      print("NOTWORKING ERROR($selcode): Test failed.  Maybe the bug is fixed?\n");
      failed($selcode, 4);
    }
  }
  else {
    if ($code == 0) {
      print("ERROR($selcode): expected this to fail:\n",
            "  ", join(' ', @args), "\n");
      failed($selcode, 4);
    }
    else {
      print("$selcode: failed as expected (code $code)\n");
      if (!$keepTemps) {
        unlink($tempfname);
      }
    }
  }
}

print("\n$me: ");

if ($contin && @failures) {
  print("failures: @failures\n");
  exit(4);
}
elsif (!$selectedError) {
  print("success: all $testedVariations variations failed as expected\n");
}
elsif ($testedVariations) {
  print("success: error $selectedError failed as expected\n");
}
else {
  print("There is no error $selectedError in $fname.\n");
}

exit(0);


# like 'system', except return a proper exit code, and
# propagate fatal signals (esp. ctrl-C)
sub mysystem {
  my @args = @_;

  my $code = system(@args);
  if ($code == 0) { return $code; }

  my $sig = $code & 127;
  if ($sig != 0) {
    if ($sig == $sigquit || $sig == $sigint) {
      # subprocess died to user-originated signal; kill myself
      # the same way
      #print("killing myself with $sig\n");
      kill $sig, $$;
    }

    # some other signal
    die("child died with signal $signame[$sig]\n");
  }

  return $code >> 8;
}


# EOF

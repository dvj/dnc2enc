#!/usr/bin/perl
my $s = "
#include \"buildtables.h\"
using namespace std;


void BuildTables(map<string,int> *m) {
";
  
while (<>) {
  chomp;
  $geo = "";
  m/^([^,]*),([^,]*),([^,]*),([^,]*)/;
  if ($2 eq 0) {
    $geo = "Point";
  } elsif ($2 eq 1) {
    $geo = "Line";
  } else {
    $geo = "Area";
  }
  $s .= "     (*m)[\"" . $1."_".$geo . "\"] = " . $4 . ";\n";
}

$s .= "   return;\n}";

#$s = "234";
print $s;

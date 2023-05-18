#!/usr/bin/ruby

# replaces lowercase identifiers with an underscore to eliminate the underscore.
# e.g., foo_bar_baz --> foobarbaz.
# tries to be minimally smart about it; will not replace symbols that look like functions, or
# are used like functions.
# BAD* also include blacklisting patterns

BADNAMES = %w(
  uint8_t
  uint16_t
  uint32_t
  uint64_t
  va_list
  size_t
  
)

BADPATS = [
  # match any symbol ending in "_t", i.e., size_t, uint8_t, etc
  /\b\w+_t\b/,

  # match any symbol starting with pcre2, i.e., pcre2_match_context, etc
  /\bpcre2\w+/,

  # match any symbol starting with tm | tv | st, i.e., st_size (of struct stat), et al
  /\b(tm|tv|st)_\w+\b/,
]

# this is where the file content is kept.
$fdata = nil

# check if the symbol is some kind of reserved word
def isreserved(sym)
  if BADNAMES.include?(sym) then
    return true
  end
  BADPATS.each do |pat|
    if sym.match?(pat) then
      return true
    end
  end
  return false
end

# check if the symbol is ok to process.
# returns false if the symbol must remain as-is.
def isok(sym)
  if isreserved(sym) then
    return false
  end
  # check that this symbol isn't also used as some sort of function:
  # reason being that a symbol like "foo" could be used as a variable
  # (i.e., acme_t func = foo)
  pat = /\b#{sym}\b\s*\(/
  if $fdata.match?(pat) then
    return false
  end
  return true
end

# callback to gsub
def dogsub(sym)
  if isok(sym) then
    # more modifications could be done here.
    # so as long as isok() returns true, anything's possible.
    nsym = sym.gsub(/_/, "")
    $stderr.printf("replacing %p --> %p\n", sym, nsym)
    return nsym
  end
  return sym
end

begin
  $fdata = File.read(ARGV[0])
  $fdata.gsub!(/\b([a-z]\w+_\w+)\b/){|sym| dogsub(sym) }
  puts($fdata)
end



#!/usr/bin/ruby

class Underscore
  def initialize
    @cachedata = {}
    @files = []
    @symtab = {}
    @files = Dir.glob("*.{h,c}")
  end

  def getdata(file)
    if @cachedata.key?(file) then
      return @cachedata[file]
    end
    data = File.read(file)
    @cachedata[file] = data
    return data
  end

  def cleanup(sym)
    nsym = sym.dup
    nsym = sym.gsub(/_/, "")
    return nsym
  end

  def issomecallable_infile(sym, file)
    d = getdata(file)
    arx = /\b#{sym}\b\s*\(/
    brx = /(->|\.)\b#{sym}\b/
    if d.match?(arx) || d.match?(brx) then
      return true
    end
    return false
  end

  def issomecallable(sym)
    @files.each do |file|
      if issomecallable_infile(sym, file) then
        return true
      end
    end
    return false
  end

  BADPATS = [
    /\b\w+_t\b/,
    /\bsi_\w+\b/,
    /\bva_\w+\b/,
    /jmp_buf/,
  ]

  REPLACEME = {


  }

  def whatwewant(sym)
    BADPATS.each do |bp|
      if sym.match?(bp) then
        return false
      end
    end
    if sym.match?(/\b[A-Z]+(\d*)?_/) || sym.match(/^_/) then
      return false
    end
    if sym.match?(/\w+_\w+/) then
      return !issomecallable(sym)
    end
    return false
  end

  def findinfile(file)
    rx = /\b(?<ident>\w{3,})\b/
    $stderr.printf("now processing %p ...\n", file)
    d = getdata(file)
    d.enum_for(:scan, rx).map{ Regexp.last_match }.each do |m|
      ident = m["ident"]
      if whatwewant(ident) then
        if !@symtab.key?(ident) then
          @symtab[ident] = 0
        end
        @symtab[ident] += 1
      end
    end
  end

  def main
    @files.each do |f|
      findinfile(f)
    end
    @symtab.each do |sym, cnt|
      #printf("  %p => %p,\n", sym, sym.gsub(/_/, ""))
    end
    $stderr.printf("now replacing ...\n")
    @files.each do |file|
      d = File.read(file)
      totalcnt = 0
      @symtab.each do |sym, cnt|
        rep = REPLACEME[sym]
        if rep == nil then
          rep = cleanup(sym)
        end

        $stderr.printf("replacing %p --> %p\n", sym, rep)
        rx = /\b#{sym}\b/
        if d.match?(rx) then
          totalcnt += 1
          d.gsub!(rx, rep)
        end
        
      end
      if totalcnt > 0 then
        $stderr.printf("replacing %d identifiers in %p\n", totalcnt, file)
        File.write(file, d)
      end
    end
  end

end


begin
  u = Underscore.new
  u.main
end

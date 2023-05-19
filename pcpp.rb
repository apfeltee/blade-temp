#!/usr/bin/ruby -w

# parses preprocessor lines, albeit without touching comments (latter only for now).
# mostly only necessary to grep for old stuff.

class ProcPP

  # '#'
  CH_POUND = 35

  # '\\'
  CH_CONT = 92

  # '\n'
  CH_LINEFEED = 10

  # '"'
  CH_STRDOUBLEQUOTE = 34

  # '\''
  CH_STRSINGLEQUOTE = 39

  # -1=EOF, linefeed, space, tab, carriage return
  SPACES = [-1, CH_LINEFEED, ' '.ord, "\t".ord, "\r".ord]

  def initialize()
    @stream = nil
    @inmacro = false
    @indoublestring = false
    @insinglestring = false
    @parentwasblank = true
    @fileline = 1
    @rawmacro = ""
    @chnow = -1
    @chprev = -1
    @chnext = -1
  end

  def fromfile(path)
    File.open(path, "rb") do |ofh|
      fromstream(ofh)
    end
  end

  def forward()
    @chprevprev = @chprev
    @chprev = @chnow
    if @chnext == -1 then
      @chnow = @stream.getbyte
      @chnext = @stream.getbyte
    else
      @chnow = @chnext
      @chnext = @stream.getbyte
    end
    if @chnow == CH_LINEFEED then
      @fileline += 1
    end
    @parentwasblank = SPACES.include?(@chprev)
    # these aren't used *yet*, but in order to process comments,
    # it's necessary to know when inside a string
    # think `printf("this is/* not*/ a //comment");`
    # without tracking string begin and end (no need to actually store anything, by the way),
    # this would result in broken output.
    if !@insinglestring then
      if (@chnow == CH_STRDOUBLEQUOTE) && (@chprev != CH_CONT) then
        @indoublestring = !@indoublestring
      end
    end
    if !@indoublestring then
      if (@chnow == CH_STRSINGLEQUOTE) && (@chprev != CH_CONT) then
        @insinglestring = !@insinglestring
      end
    end
  end

  def fromstream(ss)
    @stream = ss
    while true do
      forward()
      if @chnow == nil then
        break
      end
      # the idea is to check if whatever came before a '#' can only
      # be space (including linefeeds, obviously).
      # @parentwasblank will always be reset upon a linefeed, for this reason.
      if !SPACES.include?(@chprev)
        @parentwasblank = false
      end
      if @inmacro then
        @rawmacro += @chnow.chr
        if (@chnow == CH_CONT) && (@chnext == CH_LINEFEED) then
          # eat CH_CONT
          forward()
          # eat CH_LINEFEED
          if @chnow != CH_LINEFEED then
            # how much sense does this really make?
            #raise sprintf("near line %d: expected linefeed after '\\', but got a %p (%d)", @fileline, @chnow.chr, @chnow)
          end
          forward()
        elsif @chnow == CH_LINEFEED then
          @inmacro = false
          @parentwasblank = true
          emitmacro()
          @rawmacro = ""
        end
      else
        if @parentwasblank && (@chnow == CH_POUND) then
          @inmacro = true
        end
      end
    end
  end

  def emitmacro()
    #bits = @rawmacro.split(/\s/).map(&:strip).reject(&:empty?)
    $stderr.printf("rawmacro (near %d): %p\n", @fileline - 1, @rawmacro)
    #printf("#%s", @rawmacro)
    #if @rawmacro[-1] != "\n" then
    # puts()
    #end
  end

end

begin
  cpp = ProcPP.new
  cpp.fromfile(ARGV[0])
end



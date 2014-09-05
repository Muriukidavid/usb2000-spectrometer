#!/usr/bin/wish
#
# script to display continuously a spectrum from the USB2000 device

set dataroot [pwd]
set programroot [pwd]
set sd [pwd] ; set sdl $sd ; # saving directory
set program "$programroot/spectroread"
set tmpfile "$dataroot/.tempspectrum"
set gnucontrol "$dataroot/monitor.gnu" ; # for gnu file
set gnucanvas "$dataroot/gcan" ; # canvas file
set XMIN 350
set XMAX 1000
set YMIN ""
set YMAX ""
set TIMEPERBIN 100 ; # integration time in millisec
set tipeb $TIMEPERBIN
set VERBOSITY 31 ; # verbosity for spectrum
set RUNNING 0
set YSCALE 1

# title text
set FNT "-*-Helvetica-*-r-*-*-18-180-*-*-*-*-*-*"
label .headline -text "USB2000/2000+ spectrometer" -font $FNT
pack .headline -anchor w

# prepare buttons
# main button tree
frame .mainknobs
button .mainknobs.start -text "start" -command runproc
bind .mainknobs.start <Return> {.mainknobs.start invoke}
pack .mainknobs.start -side left
button .mainknobs.stop -text "stop" -command haltbutton
bind .mainknobs.stop <Return> {.mainknobs.stop invoke}
pack .mainknobs.stop -side left
button .mainknobs.single -text "single" -command singlebutton
bind .mainknobs.single <Return> {.mainknobs.single invoke}
pack .mainknobs.single -side left

button .mainknobs.save -text "save" -command savebutton
button .mainknobs.exit -text "exit" -command exitbutton
button .mainknobs.print -text "print" -command printbutton
bind .mainknobs.save <Return> {.mainknobs.save invoke}
bind .mainknobs.exit <Return> {.mainknobs.exit invoke}
bind .mainknobs.print <Return> {.mainknobs.pint invoke}
pack .mainknobs.save .mainknobs.exit .mainknobs.print -side left

pack .mainknobs -anchor w

# entry for time per bin
frame .tiperbin
label .tiperbin.txt -text "integration time in msec: "
entry .tiperbin.entry -width 10 -relief sunken -bd 2 -textvariable tipeb
bind .tiperbin.entry <Return> {
if {($tipeb >= 10000) || ($tipeb < 1)} {
set tipeb $TIMEPERBIN
}
set TIMEPERBIN $tipeb
}
pack .tiperbin.txt .tiperbin.entry -side left
pack .tiperbin


# axis options
frame .axops
button .axops.taxis -text "lambda axis" -command taxisbutton
button .axops.yaxis -text "y axis" -command yaxisbutton
bind .axops.taxis <Return> {.axops.taxis invoke}
bind .axops.yaxis <Return> {.axops.yaxis invoke}
pack .axops.taxis .axops.yaxis -side left
pack .axops -anchor w

# canvas for display
canvas .1 ; # -bg white -width 500 -height 300
pack .1

# -------------------------------------------------------------
# working procedures

# procedures for axis control
proc taxisbutton {} {
global XMIN XMAX
set tmi $XMIN ; set tma $XMAX
if {[winfo exists .tax]} {destroy .tax ; return}
frame .tax -relief ridge -borderwidth 2
button .tax.auto -text "auto" -command {
set tmi [set XMIN ""] ; set tma [set XMAX ""] ; updateboth }
label .tax.l1 -text "lambda start: "
entry .tax.e1 -width 10 -relief sunken -bd 2 -textvariable tmi
bind .tax.e1 <Return> { if {$tmi < $XMAX } {
set XMIN $tmi ; updateboth } else { set tmi $XMIN } }
label .tax.l2 -text "lambda end: "
entry .tax.e2 -width 10 -relief sunken -bd 2 -textvariable tma
bind .tax.e2 <Return> { if {$tma > $XMIN } {
set XMAX $tma ; updateboth } else { set tma $XMAX } }
button .tax.ok -text "ok" -command {destroy .tax}
pack .tax.auto .tax.l1 .tax.e1 .tax.l2 .tax.e2 .tax.ok -side left
pack .tax
}

proc yaxisbutton {} {
global YMIN YMAX YSCALE
set ymi $YMIN ; set yma $YMAX
if {[winfo exists .yax]} {destroy .yax ; return}
frame .yax -relief ridge -borderwidth 2 ; frame .yax.1 ; frame .yax.2
button .yax.1.auto -text "auto" -command {
set ymi [set YMIN ""] ; set yma [set YMAX ""] ; updateboth }
label .yax.1.l1 -text "y start: "
entry .yax.1.e1 -width 10 -relief sunken -bd 2 -textvariable ymi
bind .yax.1.e1 <Return> { if {$ymi < $YMAX } {
set YMIN $ymi ; updateboth } else { set ymi $YMIN } }
label .yax.1.l2 -text "y end: "
entry .yax.1.e2 -width 10 -relief sunken -bd 2 -textvariable yma
bind .yax.1.e2 <Return> { if {$yma > $YMIN } {
set YMAX $yma ; updateboth } else { set yma $YMAX } }
button .yax.1.ok -text "ok" -command {destroy .yax}
set YSCALE 1
radiobutton .yax.2.lin -text "lin scale" -variable YSCALE -value 1 \
-command { updateboth }
radiobutton .yax.2.log -text "log scale" -variable YSCALE -value 2 \
-command { updateboth }
pack .yax.1.auto .yax.1.l1 .yax.1.e1 .yax.1.l2 .yax.1.e2 .yax.1.ok \
-side left
pack .yax.2.lin .yax.2.log -side left
pack .yax.1 .yax.2 ; pack .yax
}


# procedure for halt button
proc haltbutton {} {
global RUNNING
set RUNNING 0
}


# procedure to loop during run - start button
proc runproc {} {
global program RUNNING TIMEPERBIN tmpfile VERBOSITY
if {$RUNNING != 0} return
set RUNNING 1
set returnvalue 0
.mainknobs.single configure -state disabled
.mainknobs.start configure -relief sunken
.mainknobs.save configure -state disabled
.mainknobs.print configure -state disabled
if {[winfo exist .axops.etxt]} {destroy .axops.etxt}

while {[expr ($RUNNING != 0) ]} {
set returnvalue [ catch { exec $program -o $tmpfile -i $TIMEPERBIN -V $VERBOSITY } evar ]

if {$returnvalue == 0 } { ; # we can safely plot now
updategraphics;
} else {
set RUNNING 0
label .axops.etxt -text "Spectrometer error. Try again." -foreground red
pack .axops.etxt -side left
}
update
}
.mainknobs.save configure -state normal
.mainknobs.print configure -state normal
.mainknobs.start configure -relief raised
.mainknobs.single configure -state normal
}

proc singlebutton {} {
global program RUNNING TIMEPERBIN tmpfile VERBOSITY
if {$RUNNING != 0} return
set returnvalue 0
.mainknobs.start configure -state disabled
.mainknobs.single configure -relief sunken
if {[winfo exist .axops.etxt]} {destroy .axops.etxt}

set returnvalue [ catch { exec $program -o $tmpfile -i $TIMEPERBIN -V $VERBOSITY } ]

if {$returnvalue == 0 } { ; # we can safely plot now
updategraphics;
} else {
label .axops.etxt -text "Spectrometer Error. Try again" -foreground red
pack .axops.etxt -side left
}

update
.mainknobs.single configure -relief raised
.mainknobs.start configure -state normal
}

# procedure save button
proc savebutton {} {
global sd tmpfile
.mainknobs.start configure -state disabled

if {[winfo exists .fsel]} return
global fname COMMENT SAVED TIMEPERBIN BINNUM nru
if {[getfilename] == ""} return
exec cat $tmpfile >$sd/$fname
set outfile [open $sd/$fname a+ ]
puts $outfile "# comment on this run: $COMMENT"
close $outfile
set SAVED 1
}


proc getfilename {} {
global fname COMMENT sd sdl
frame .fsel -relief ridge -borderwidth 2
frame .fsel.dir
frame .fsel.three
frame .fsel.one
frame .fsel.two
label .fsel.dir.t -text "current directory: "
entry .fsel.dir.e -relief sunken -width 20 -bd 2 -textvariable sdl
.fsel.dir.e xview moveto 1
bind .fsel.dir.e <Return> {
if {[file isdirectory $sdl]} {set sd $sdl } else {set sdl $sd}
}
label .fsel.one.t -text "Enter file name: "
entry .fsel.one.e -relief sunken -width 20 -bd 2 -textvariable fname
label .fsel.three.t -text "current comment: "
entry .fsel.three.e -relief sunken -width 20 -bd 2 -textvariable COMMENT
bind .fsel.one.e <Return> {
if {[file exists $fname]} {
pack .fsel.two.w .fsel.two.t .fsel.two.b -side left
} else { destroy .fsel }
}
label .fsel.two.w -bitmap warning
label .fsel.two.t -text " File exists "
button .fsel.two.b -text "Overwrite" -command { destroy .fsel }
button .fsel.two.c -text "Cancel" -command {
set fname "" ; destroy .fsel
.mainknobs.start configure -state normal
}
bind .fsel.two.c <Return> {.fsel.two.c invoke}
pack .fsel.dir.t .fsel.dir.e -side left
pack .fsel.two.c -side right
pack .fsel.one.t .fsel.one.e -side left
pack .fsel.three.t .fsel.three.e -side left
pack .fsel.dir .fsel.three .fsel.one .fsel.two
pack .fsel
tkwait window .fsel
return $fname
}


# printer button
set LP "lp"
proc printbutton {} {
global LP
if {[winfo exists .pri]} return
.mainknobs.start configure -state disabled

frame .pri -relief ridge -borderwidth 2
frame .pri.one ; frame .pri.two ; frame .pri.three
label .pri.one.t -text "PostScript printer: "
entry .pri.one.e -width 5 -relief sunken -bd 2 -textvariable LP
button .pri.one.b -text "Print" -command {
gnups $LP ; destroy .pri ; return
}
label .pri.two.t -text "eps file : "
entry .pri.two.e -width 5 -relief sunken -bd 2 -textvariable efna
button .pri.two.b -text "save to" -command {
gnueps $efna ; destroy .pri ; return
}
button .pri.three.b -text "cancel" -command {
destroy .pri
.mainknobs.start configure -state normal
return
}
pack .pri.one.t .pri.one.e .pri.one.b -side left
pack .pri.two.t .pri.two.e .pri.two.b -side left
pack .pri.three.b -side left
pack .pri.one .pri.two .pri.three -side top
pack .pri
}

# procedure exit button
proc exitbutton {} {
global tmpfile gnucanvas gnucontrol
exec rm -f $tmpfile
exec rm -f $gnucanvas
exec rm -f $gnucontrol
exit
}
bind .mainknobs.exit <Return> exit



# procedure to update paramters on canvas control file
proc updatecontrolfile { } {
global gnucontrol gnucanvas XMIN XMAX YMIN YMAX YSCALE tmpfile
exec echo "set terminal tkcanvas" >$gnucontrol
exec echo "set output \"$gnucanvas\" " >>$gnucontrol
exec echo "set xrange \[$XMIN:$XMAX\]" >>$gnucontrol
exec echo "set yrange \[$YMIN:$YMAX\]" >>$gnucontrol
if { $YSCALE == 2 } { exec echo "set logscale y " >>$gnucontrol }
exec echo "plot \'$tmpfile\' using 2:4 w l notitle " >>$gnucontrol
}
updatecontrolfile

proc updategraphics { } {
global gnucanvas gnucontrol
catch {exec gnuplot $gnucontrol }
source $gnucanvas ; # load canvas source
gnuplot .1
update
}

proc updateboth {} {
updatecontrolfile
updategraphics
}

proc gnups {printer} {
global gnucontrol gnucanvas XMIN XMAX YMIN YMAX YSCALE tmpfile
exec echo "set terminal postscript color \"Helvetica\" 18 " >$gnucontrol
exec echo "set output \"|lpr -P$printer \" " >>$gnucontrol
exec echo "set xrange \[$XMIN:$XMAX\]" >>$gnucontrol
exec echo "set yrange \[$YMIN:$YMAX\]" >>$gnucontrol
if { $YSCALE == 2 } { exec echo "set logscale y " >>$gnucontrol }
exec echo "plot \'$tmpfile\' using 2:4 w l notitle " >>$gnucontrol
catch { exec gnuplot $gnucontrol }
# restore control file
updatecontrolfile
}

proc gnueps {file} {
global gnucontrol gnucanvas XMIN XMAX YMIN YMAX YSCALE tmpfile sd
exec echo "set terminal postscript eps color size 12cm,8cm \"Helvetica\" 14 " >$gnucontrol
exec echo "set output \"$sd/$file\"" >>$gnucontrol
exec echo "set xrange \[$XMIN:$XMAX\]" >>$gnucontrol
exec echo "set yrange \[$YMIN:$YMAX\]" >>$gnucontrol
if { $YSCALE == 2 } { exec echo "set logscale y " >>$gnucontrol }
exec echo "plot \'$tmpfile\' using 2:4 w l notitle " >>$gnucontrol
catch { exec gnuplot $gnucontrol }
# restore control file
updatecontrolfile
}

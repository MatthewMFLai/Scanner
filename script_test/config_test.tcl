proc serial_set {comport} {
	set serial [open $comport r+]
	fconfigure $serial -mode "57600,n,8,1" -blocking 0 -buffering none -translation binary
	fileevent $serial readable [list serial_receiver $serial]
	return $serial
}

proc serial_receiver { chan } {
     if { [eof $chan] } {
         puts stderr "Closing $chan"
         catch {close $chan}
         return
     }
     set data [read $chan]
     set size [string length $data]
     puts "received $size bytes: $data"
}
 
proc config_parse {filename} {
	set rc ""
    if {![file exists $filename]} {
		puts "$filename not found!"
		return $rc
	}
	
	set fd [open $filename r]
	while {[gets $fd line] > -1} {
		if {$line == ""} {
			continue
		}
		set line [string trim $line]
		if {[string index $line 0] == "#"} {
			continue
		}
		append rc $line
		append rc "\n"
	}
	close $fd
	return $rc
 }
 
 proc config_send {fd filename} {
	set configbody [config_parse $filename]
	if {$configbody == ""} {
		puts "config file empty"
		return
	}
	# Extract version from config file content
	set searchstr "vers="
	set idx [string first $searchstr $configbody]
	if {$idx == -1} {
		puts "Version data not found!"
		return
	}
	incr idx [string length $searchstr]
	set idx2 [string first "\n" $configbody $idx]
	incr idx2 -1
	set version [string range $configbody $idx $idx2]
	
	puts -nonewline $fd "at\$cfgset $version [string length $configbody] $configbody"
	puts -nonewline $fd "\r"
	#puts -nonewline "at\$cfgset $version [string length $configbody] $configbody"	
	return
 }

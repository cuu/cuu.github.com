(constant 'SIGINT 2) ;; on unix system
(constant 'RMAX 128) ;; read max 

(load "func.lsp") ;; some predefined functions

(setq baud 38400)

(setq dev "/dev/ttyUSB0")

(if-not (file? dev)
	(begin
		(print "no tty found")
		(exit)
	)
)

(setq rw_iner 100)


(setq cmd (string "stty -F " dev " cs8 " baud " ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts min 0 time 70"))
;; time 30 == 3 secs ,read timeout 
;(println cmd)
(exec cmd)
(sleep 3000)
;(setq usbdev (open dev "u"))
;(close usbdev)
(print dev " ready")


(setq usbdev (open dev "u"))

(if (nil? usbdev)
	(begin
		(println "open " dev " failed")
		(exit 0)
	)
)

(define (run_serial query len)
	(setq nw (write usbdev query  len))
	(setq ret "")
	(setq nr 0)
	(sleep rw_iner)
	(if-not (nil? nw)	
		(setq nr (read usbdev buff RMAX))
	)
	(if (> nr 0)
		(begin
			(setq upk_str (dup "b "  nr ))
			(setq upk_lst (unpack upk_str buff))

			(dolist (y upk_lst)
				(extend ret (format "%x " y))
			)
			(chop ret 1)
		)
		"read has no return"
	)
;;	(println "")
)

(define (server_exit)
	(if (nil? (close usbdev) )
			(close usbdev)
	)
	(print "bye")
	;(exit)
)

(define (test str)
    (setq str_lst (parse str " "))
	(setq len (length str_lst))
    (setq ret "")
    (dolist (x str_lst)
        (setq tmp_str (slice x 0 2)) ;; force the input to be XX
        (extend ret (char (int tmp_str 0 16)))
    )
    (run_serial ret len)

)


(define (ctrlC-handler)
;;  (signal SIGINT "reset")
;   (sleep 500)
    (server_exit)
 ;   (sleep 400)
;	(signal SIGINT "reset")
    (exit)
)

(signal SIGINT ctrlC-handler)


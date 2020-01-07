#!/usr/bin/newlisp

(constant 'SIGINT 2) ;; on unix system
(constant 'RMAX 128) ;; read max 

(load "func.lsp") ;; some predefined functions

(define (myexit)
	(abort)
	(newlisp_exit)
)

(constant 'newlisp_exit exit)

(constant 'exit myexit)

(setq dev "/dev/ttyUSB0")

(if-not (file? dev)
	(begin
		(print "no tty found")
		(newlisp_exit)
	)
)

;(setq port 4711)
;(setq socket (net-listen port))
;(if (nil? socket) 
;	(begin
;		(print (net-error)) (newlisp_exit)
;	)
;)


(setq stop_flag 0)

(setq rw_iner 100)

(setq baud 38400)
(setq cmd (string "stty -F " dev " cs8 " baud " ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts min 1 time 20")) ;; time 30
;; time 30 == 3 secs ,read timeout 
;(println cmd)
(exec cmd)
(sleep 3000)
(println dev " ready")


(setq usbdev (open dev "u"))
(if (nil? usbdev)
	(begin
		(println "open " dev " failed")
		(exit 0)
	)
)

(define (write_serial query len )
		(setq nw 0)
		(setq nw (write usbdev query len))
;;		nw	
)

(define (read_serial )
	(setq ret "")
	(setq nr 0)
;;	(sleep rw_iner)
;;	(if-not (nil? nw)	
	(setq nr (read usbdev buff RMAX))
;;	)

	(if (> nr 0)
		(begin
			(setq upk_str (dup "b "  nr ))
			(setq upk_lst (unpack upk_str buff))

			(dolist (y upk_lst)
				(extend ret (format "%x " y))
			)

			;(net-send server (chop ret 1))
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
;	(close socket)
	(print "bye")
	(newlisp_exit)
)

(define (test str)
    (setq str_lst (parse str " "))
	(setq len (length str_lst))
    (setq ret "")
    (dolist (x str_lst)
        (setq tmp_str (slice x 0 2)) ;; force the input to be XX
        (extend ret (char (int tmp_str 0 16)))
    )
    (write_serial ret len)
		
)


(define (ctrlC-handler)
;;  (signal SIGINT "reset")
;   (sleep 500)
    (server_exit)
 ;   (sleep 400)
;	(signal SIGINT "reset")
	(setq stop_flag 1)
    (newlisp_exit)

)

(signal SIGINT ctrlC-handler)

(define (read_proc)
	(until  (= stop_flag 1)
			(setq read_ret (read_serial))
			(println read_ret)
	)
	(newlisp_exit)	
)


(setq cpid (spawn 'result (read_proc) true) )

;(while (setq server (net-accept socket))
;	(if (not (net-select server "read" 10000))
;		(begin
;			(if (net-error) (print (net-error)))
;			(print "net-select server read error")
;		)
;		(begin
;			(net-receive server buffer 4096)
;			(eval-string buffer)
;			(sleep 300)
;		)
;	)
;
;	(net-close server)
;	(setq server nil)
;
;)

;;;;; here sscanf input ,to write_serial
(while (read-line)
	(if (= (current-line) "quit")
		(exit)
		(test (current-line))
	)
)


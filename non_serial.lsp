(constant 'SIGINT 2) ;; on unix system
(constant 'RMAX 128) ;; read max 

(setq create nil)

(dolist (x (main-args) (if (= x "-r") (begin (setq create true) true)) ))

;;https://github.com/cormullion/newlisp-projects
(load "newlisp-projects/nldb/nldb.lsp");; nldb database support
(load "func.lsp") ;; some predefined functions

;(import "libc.so.6"  "select")

(define (myexit n)
;;    (abort)
    (newlisp_exit n)
)

(constant 'newlisp_exit exit)
(constant 'exit myexit)

(setq stop_flag 0)

(setq dev "/dev/ttyUSB0")

(if-not (file? dev)
	(begin
		(println "no tty found")
		(exit 0)
	)
)
;;;;
(define (init_kv)
	(new Tree 'CKV)
	(save "CKV.lsp" 'CKV);;; CUU KEY-VALUE database	
)

(define (init_errors)
	(nldb:create-table 'errors '(saddr error_type timestamp)) ;;; error_type forexample 1: adc error 2:.... 
	(nldb:save-db "errors.lsp")
)

(if-not (file? "CKV.lsp")
	(init_kv);;init databases
;	(load "addrs.nldb")
)
(if-not (file? "errors.lsp")
	(init_errors);;; errors.lsp store error reports
)

(load "CKV.lsp")
(load "errors.lsp")
;;;;

(set 'talk (share))     (share talk 0)
(set 'content (share))  (share content "")

(setq cur_par "0")

(setq rw_iner 200)

(setq baud 38400)
(setq cmd (string "stty -F " dev " cs8 " baud " ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts min 0 time 30"))
;; time 30 == 3 secs ,read timeout 
;(println cmd)
(exec cmd)
(sleep 3000)
;(setq usbdev (open dev "u"))
;(close usbdev)
(println dev " ready")


;(setq rusbdev (open dev "r" "n"))
;(if (nil? rusbdev)
;	(begin
;		(println "open " dev " read failed")
;		(exit rusbdev)
;	)
;)

(setq wusbdev (open dev "w" "n"))
(if (nil? wusbdev)
	(begin
		(println "open " dev " for write failed")
		(exit wusbdev)
	)
)

(define (read_serial read_fd)
    (setq ret "read has no return ")
    (setq nr 0)
    (setq nr (read read_fd buff RMAX))
	
;	(println "nr " nr)	
	(if (> nr 0)
		(begin
			(setq ret "")
			(share talk 1) ;; has return
;			(println "set talk to " (share talk))
		)
	)	
    (while (> nr 0)
        
            (setq upk_str (dup "b "  nr ))
            (setq upk_lst (unpack upk_str buff))

            (dolist (y upk_lst)
                (extend ret (format "%x " y))
            )
			(setq nr (read read_fd buff RMAX))
;;			(chop ret 1)
    )

;;	(chop ret 1)
	ret
)

(define (run_serial query len timeout)
	(setq ret "")
	(share content "")
	(setq nw (write wusbdev query  len))
	(setq tmp_lst nil)

;;	(println "into run_serial " query)
	(sleep timeout);; give enought time for reading
;;	(println "sleeped" timeout)
	(if-not (nil? nw)
		(begin
;			(println (share talk))	
			(if (= (share talk) 1) ;; read process has something to return
					(begin
						(share talk 0)
						
						(setq ret (share content))
						;(println "1st " (share content))
						(if (and (= (nth 0 (share content)) "c" ) (= (nth 1 (share content)) "e"))
							(begin
								;(println (share content))
								(if (= (length query) 1);; ca
										(setq cur_par "00 00")			
									(= (length query) 3);; caxxxx
										(setq cur_par (string (format "%x" (char (nth 1 query))) " " (format "%x" (char (nth 2 query)))) )
								)
								;(println "cur_par " cur_par)
								(setq tmp_lst (parse (share content) " "));;;
								;(println tmp_lst)
								(if (> (int (nth 1 tmp_lst) 0 16) 0)
									(begin
										;(println "1st tmp_lst > 0")
										(for (x 2 (- (length tmp_lst) 1) 2  (> x (- (length tmp_lst) 1)) )
											;(println "x= " x)
											(setq short_addr (string (nth x tmp_lst) " " (nth (+ x 1) tmp_lst)) )
											
										;;	(if (nil? (CKV short_addr) )
												(CKV short_addr (list nil cur_par (date-value)) )
										;;	)
											;(println "test ca " short_addr)
											(test (string "ca " short_addr) rw_iner)
										)	
									)
									;	(println "1st tmp_lst <= 0")
								)

								(save "CKV.lsp" 'CKV);;; next time,use (CKV) for all short address,then get live mac address, stored in CKV,the all macs 
								;; can be listed to users;; or! still use short address to be listed, both ways can work
							)
						)
						(if (and (= (nth 0 (share content)) "a") (= (nth 1 (share content)) "c")) ;; ac is response for "m" which is 0x6d
							(begin
								(setq tmp_lst (parse (share content) " "))
								(setq short_addr (string (nth 1 tmp_lst) " " (nth 2 tmp_lst)))
								(setq short_addr_ckv (CKV short_addr))
								(setq (nth 0  short_addr_ckv) (chop  (join (map string (slice tmp_lst 3) ) " ") 1) )
								(CKV short_addr short_addr_ckv)
								(save "CKV.lsp" 'CKV)
							)
						)
						;(println "in the last")	
						(share content "")
						ret
					)
					"talk is 0"
			)
		)
		"wrote nothing"	
	)
)

(define (server_exit)
	(if (nil? (close wusbdev) )
			(close wusbdev)
	)
	(print "bye")
	;(exit)
)

(define (test str time_out)
	;; non_block read process
	(if (nil? time_out)
			(setq time_out 500)
	)
;	(println str " + " time_out)
	(setq str (replace "  " str " "))	
    (setq str_lst (parse str " "))
	(setq len (length str_lst))
    (setq ret "")
    (dolist (x str_lst)
        (setq tmp_str (slice x 0 2)) ;; force the input to be XX
        (extend ret (char (int tmp_str 0 16)))
    )
;	(println "run_serial")
	
    (run_serial ret len time_out)
;;	(share content "")
)


(define (ctrlC-handler)
;;  (signal SIGINT "reset")
;   (sleep 500)
    (server_exit)
 ;   (sleep 400)
;	(signal SIGINT "reset")
	(share talk 3)
    (exit 0)
)

(signal SIGINT ctrlC-handler)

(define (read_proc)

	(setq rusbdev (open dev "r" "n"))
	(if (nil? rusbdev)
   		(begin
       		(println "open " dev " read failed")
       		(exit rusbdev)
	   )
	)

    (until  (= (share talk) 3)  ;; for stoping and exiting
	;;		(while (and (net-select rusbdev "read" 10000) (<= (peek rusbdev) 0) )
			(while (not (net-select rusbdev "read" 10000) ))
			;;	(println "net-select")
;;				(sleep 30)
;			)
;			(while (= (peek rusbdev) 0)
;				(sleep 30)
;			)
				(setq read_ret (read_serial rusbdev) )
				;;check commands
					
	           	(share content (append (share content) read_ret) )
            	(println "in read_proc " (date-value) " "  (share content))
;;			)
;;			(sleep rw_iner)
    )
	(close rusbdev)
    (exit 0)
)

(setq cpid (fork (read_proc)) )
(print cpid)

;(wait-pid cpid)
;(sleep 3000)
;(println "final: " (test "fe 00 21 01 20" 100) )

;(println "final: " (test "fe 00 21 01 20" 100) )

;(println "final: " (test "fe 00 21 01 20" 100) )

;(share talk 3) (wait-pid cpid)
(define (get_all_macs)
	(dolist (x (CKV))
		(test (string "6d " (nth 0 x))  200)
	)
)

(if (true? create)
	(begin
		(test "ca" 200)
		(sleep 500)
		(println "test ca is over")
		(get_all_macs)
	)
)


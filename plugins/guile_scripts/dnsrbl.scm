;;
;; Plugin to check multiple DNS realtime block lists and
;; trap the address if it is present.
;;

;; TODO: load list of rbls from config
(let ((rbls '(sbl.spamhaus.org)))
  (register-spamtrap
   "dnsrbl-trap"
   (lambda (ip helo from to)
     (check-all-rbls ip rbls))))

(define (check-all-rbls ip rbls)
  (if (> (length rbls) 0)
      (if (= (check-rbl ip (car rbls)) 0)
          (check-all-rbls ip (cdr rbls))
          1)
      0))

(define (check-rbl ip rbl)
  (host-exists? (make-rbl-lookup-host ip rbl)))

(define (host-exists? host)
  (catch 'host-not-found
    (lambda ()
      (gethost host)
      1)
    (lambda (key . params)
      (debug (format #f "DNS RBL clear for ~a" host))
      0)))

(define (make-rbl-lookup-host ip rbl)
  (format #f "~a.~a" (reverse-octets ip) rbl))

(define (reverse-octets ip)
  (string-join (reverse (string-split ip #\.)) "."))

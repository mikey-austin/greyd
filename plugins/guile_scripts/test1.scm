(info ">> Loading spamtrap guile plugin")

;; Returning 0 means the IP address should not be trapped.
(register-spamtrap
 "my-guile-never-trap-spamtrap"
 (lambda (ip helo from to)
   (debug (format #f ">> Not trapping ~a ~a ~a ~a" ip helo from to))
   0))

;; Returning 1 triggers the trap.
(register-spamtrap
 "my-guile-always-trap-spamtrap"
 (lambda (ip helo from to)
   (warn (format #f ">> Trapping ~a ~a ~a ~a" ip helo from to))
   1))

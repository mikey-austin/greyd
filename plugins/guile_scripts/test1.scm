;; My first message from guile world!
(debug "Hello world!")
(register-spamtrap
 (lambda (ip helo from)
   (debug (string-append "*** spamtrap received "
                         ip
                         helo
                         from))))

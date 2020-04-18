; To use this,
; 1) Add to init.el:
;    (setq-default chrome-root "/path/to/chrome/src/")
;    (add-to-list 'load-path (concat chrome-root "tools/emacs"))
;    (require 'trybot)
; 2) Run on trybot output:
;    M-x trybot
;
; To hack on this,
; M-x eval-buffer
; M-x trybot-test-win  or  M-x trybot-test-mac

(defvar chrome-root nil
  "Path to the src/ directory of your Chrome checkout.")

(defun get-chrome-root ()
  (or chrome-root default-directory))

; Hunt down from the top, case correcting each path component as needed.
; Currently does not keep a cache.  Returns nil if no matching file can be
; figured out.
(defun case-corrected-filename (filename)
  (save-match-data
    (let ((path-components (split-string filename "/"))
          (corrected-path (file-name-as-directory (get-chrome-root))))
      (mapc
       (function
        (lambda (elt)
          (if corrected-path
              (let ((next-component
                     (car (member-ignore-case
                           elt (directory-files corrected-path)))))
                (setq corrected-path
                      (and next-component
                           (file-name-as-directory
                            (concat corrected-path next-component))))))))
       path-components)
      (if corrected-path
          (file-relative-name (directory-file-name corrected-path)
                              (get-chrome-root))
        nil))))

(defun trybot-fixup-win ()
  "Fix up Windows-specific output."

  ; Fix Windows paths ("d:\...\src\").
  (save-excursion
    ; This regexp is subtle and rather hard to read. :~(
    ; Use regexp-builder when making changes to it.
    (while (re-search-forward
            (concat
             ; First part: path leader, either of the form
             ;   e:\...src\  or  ..\
             "\\(^.:\\\\.*\\\\src\\\\\\|\\.\\.\\\\\\)"
             ; Second part: path, followed by error message marker.
             "\\(.*?\\)[(:]") nil t)
      (replace-match "" nil t nil 1)
      ; Line now looks like:
      ;  foo\bar\baz.cc error message here
      ; We want to fixup backslashes in path into forward slashes,
      ; without modifying the error message - by matching up to the
      ; first colon above (which will be just beyond the end of the
      ; filename) we can use the end of the match as a limit.
      (subst-char-in-region (point) (match-end 0) ?\\ ?/)
      ; See if we can correct the file name casing.
      (let ((filename (buffer-substring (match-beginning 2) (match-end 2))))
        (if (and (not (file-exists-p filename))
                 (setq filename (case-corrected-filename filename)))
            (replace-match filename t t nil 2))))))

(defun trybot-fixup-maclin ()
  "Fix up Mac/Linux output."
  (save-excursion
    (while (re-search-forward "^/b/build/[^ ]*/src/" nil t)
      (replace-match ""))))

(defun trybot-fixup (type-hint)
  "Parse and fixup the contents of the current buffer as trybot output."

  ; XXX is there something I should so so this stuff doesn't end up on the
  ; undo stack?

  ;; Fixup paths.
  (cd (get-chrome-root))

  (goto-char (point-min))

  ;; Fix up path references.
  (cond ((eq type-hint 'win) (trybot-fixup-win))
        ((eq type-hint 'mac) (trybot-fixup-maclin))
        ((eq type-hint 'linux) (trybot-fixup-maclin))
        (t (trybot-fixup-win) (trybot-fixup-maclin)))

  (compilation-mode))

(defun trybot-get-new-buffer ()
  "Get a new clean buffer for trybot output."
  ; Use trybot-buffer-name if available; otherwise, "*trybot*".
  (let ((buffer-name (if (boundp 'trybot-buffer-name)
                         trybot-buffer-name
                       "*trybot*")))
    (let ((old (get-buffer buffer-name)))
      (when old (kill-buffer old)))
    (get-buffer-create buffer-name)))

(defun trybot-fetch (type-hint url)
  "Fetch a URL and postprocess it as trybot output."

  (let ((on-fetch-completion
         (lambda (process state)
           (switch-to-buffer (process-buffer process))
           (when (equal state "finished\n")
             (trybot-fixup (process-get process 'type-hint)))))
        (command (concat "curl -s " (shell-quote-argument url)
                         ; Pipe it through the output shortener.
                         (cond
                          ((eq type-hint 'win)
                           (concat " | " (get-chrome-root)
                                   "build/sanitize-win-build-log.sh"))
                          ((eq type-hint 'mac)
                           (concat " | " (get-chrome-root)
                                   "build/sanitize-mac-build-log.sh"))))))

    ; Start up the subprocess.
    (let* ((coding-system-for-read 'utf-8-dos)
           (buffer (trybot-get-new-buffer))
           (process (start-process-shell-command "curl" buffer command)))
      ; Attach the type hint to the process so we can get it back when
      ; the process completes.
      (process-put process 'type-hint type-hint)
      (set-process-query-on-exit-flag process nil)
      (set-process-sentinel process on-fetch-completion))))

(defun trybot-test (type-hint filename)
  "Load the given test data filename and do the trybot parse on it."

  (let ((trybot-buffer-name "*trybot-test*")
        (url (concat "file://" (get-chrome-root) "tools/emacs/" filename)))
    (trybot-fetch type-hint url)))

(defun trybot-test-win ()
  "Load the Windows test data and do the trybot parse on it."
  (interactive)
  (trybot-test 'win "trybot-windows.txt"))
(defun trybot-test-mac ()
  "Load the Mac test data and do the trybot parse on it."
  (interactive)
  (trybot-test 'mac "trybot-mac.txt"))
(defun trybot-test-linux ()
  "Load the Linux test data and do the trybot parse on it."
  (interactive)
  (trybot-test 'linux "trybot-linux.txt"))

(defun trybot (url)
  "Fetch a trybot URL and fix up the output into a compilation-mode buffer."
  (interactive "sURL to trybot stdout (leave empty to use clipboard): ")

  ;; Yank URL from clipboard if necessary.
  (when (= (length url) 0)
    (with-temp-buffer
      (clipboard-yank)
      (setq url (buffer-string))))

  ;; Append /text to the URL to get plain text output in the common
  ;; case of getting a URL to the HTML build log.
  (when (equal "stdio" (car (last (split-string url "/"))))
    (setq url (concat url "/text")))

  (let ((type-hint (cond ((string-match "/[Ww]in" url) 'win)
                         ((string-match "/mac/" url) 'mac)
                         ; Match /linux, /linux_view, etc.
                         ((string-match "/linux" url) 'linux)
                         (t 'unknown))))
    (trybot-fetch type-hint url)))

(provide 'trybot)

; To get syntax highlighting and tab settings for gyp(i) files, add the
; following to init.el:
;     (setq-default chrome-root "/path/to/chrome/src/")
;     (add-to-list 'load-path (concat chrome-root "tools/emacs"))
;     (require 'chrome-filetypes)

(define-derived-mode gyp-mode python-mode "Gyp"
  "Major mode for editing Generate Your Project files."
    (setq indent-tabs-mode nil
          tab-width 2
          python-indent 2))

(add-to-list 'auto-mode-alist '("\\.gyp$" . gyp-mode))
(add-to-list 'auto-mode-alist '("\\.gypi$" . gyp-mode))

(provide 'chrome-filetypes)

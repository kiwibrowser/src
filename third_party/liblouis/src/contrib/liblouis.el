;;; liblouis.el --- mode for editing liblouis translation tables

;; Copyright (C) 2009, 2011 Christian Egli

;; This file is not part of GNU Emacs.

;; GNU Emacs is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.

;; GNU Emacs is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GNU Emacs; see the file COPYING.  If not, write to the
;; Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
;; Boston, MA 02110-1301, USA.

;;; Commentary:


;;; Code:

(defvar liblouis-mode-hook nil
  "Normal hook run when entering liblouis mode.")

(defvar liblouis-mode-abbrev-table nil
  "Abbrev table in use in liblouis mode buffers.")
(define-abbrev-table 'liblouis-mode-abbrev-table ())

(eval-after-load "compile"
  '(add-to-list 'compilation-error-regexp-alist
                ;; WARNING: foo.txt: line 13: section title out of sequence: expected level 3, got level 4
                ;; ERROR: foo.txt: line 18: only book doctypes can contain level 0 sections
                '("^\\(ERROR\\|WARNING\\|DEPRECATED\\): \\([^:]*\\): line \\([0-9]+\\):" 2 3)))

(defconst liblouis-comment-regexp "\\(\\s-+.*\\)??")

(defconst liblouis-font-lock-keywords
  (list
   
   ;; Comment Lines
   (cons "^#.*$" 'font-lock-comment-face)
   
   ;; Opcodes (with one word arg)
   (list
    (concat "^"
	    (regexp-opt
	     '("include" "locale" "noletsign" "noletsignbefore" "noletsignafter"
	       "nocont" "compbrl" "literal"
	       "contraction") 'words)
	    "\\s-+\\([^       ]+\\)" liblouis-comment-regexp "$")
    '(1 font-lock-keyword-face)
    '(2 font-lock-constant-face)
    '(3 font-lock-comment-face nil t))
   
   ;; Opcodes (with one dot pattern arg)
   (list
    (concat "^"
	    (regexp-opt
	     '("numsign" "capsign" "italsign" "begital" "endital" "boldsign"
	       "begbold" "endbold" "undersign" "begunder" "endunder"
	       "firstwordital" "lastworditalbefore" "lastworditalafter"
	       "firstletterital" "lastletterital" "singleletterital"
	       "firstwordbold" "lastwordboldbefore" "lastwordboldafter"
	       "firstletterbold" "lastletterbold" "singleletterbold"
	       "firstwordunder" "lastwordunderbefore" "lastwordunderafter"
	       "firstletterunder" "lastletterunder" "singleletterunder"
	       "begcomp" "endcomp"
	       "begcaps" "endcaps" "letsign"
	       "exactdots") 'words)
	    "\\s-+\\([0-9-@]+\\)" liblouis-comment-regexp "$")
    '(1 font-lock-keyword-face)
    '(2 font-lock-constant-face)
    '(3 font-lock-comment-face nil t))
   
   ;; Opcodes (with two args)
   (list
    (concat "^"
	    (regexp-opt
	     '("comp6" "after" "before" "space" "punctuation" "digit" "lowercase" "uppercase"
	       "letter" "uplow" "litdigit" "sign" "math" "display" "repeated"  "always"
	       "nocross" "syllable" "largesign" "word" "partword" "joinnum" "joinword"
	       "lowword" "sufword" "prfword" "begword" "begmidword" "midword" "midendword"
	       "endword" "prepunc" "postpunc" "begnum" "midnum" "endnum" "decpoint" "hyphen") 'words)
	    "\\s-+\\([^       ]+?\\)\\s-+\\([-0-9=@a]+\\)" liblouis-comment-regexp "$")
    '(1 font-lock-keyword-face)
    '(2 font-lock-string-face)
    '(3 font-lock-constant-face)
    '(4 font-lock-comment-face nil t))
   
   ;; Opcodes (with two args where the second one is not a dot pattern)
   (list
    (concat "^"
	    (regexp-opt
	     '("class" "replace" "context" "correct" "pass2" "pass3" "pass4" ) 'words)
	    "\\s-+\\([^       ]+?\\)\\s-+\\([^        ]+\\)" liblouis-comment-regexp "$")
    '(1 font-lock-keyword-face)
    '(2 font-lock-string-face)
    '(3 font-lock-constant-face)
    '(4 font-lock-comment-face nil t))
					; FIXME: "multind" "lenitalphrase" lenboldphrase lenunderphrase  "swapcd" "swapdd" "capsnocont"
   )
  "Default expressions to highlight in liblouis mode.")

;;###autoload
(define-derived-mode liblouis-mode text-mode "liblouis"
  "Major mode for editing liblouis translation tables.
Turning on liblouis mode runs the normal hook `liblouis-mode-hook'.
"
  (modify-syntax-entry ?\'  ".")
  (modify-syntax-entry ?# "< b")
  (modify-syntax-entry ?\n "> b")

  (set (make-local-variable 'compile-command)
       (concat "lou_checktable " buffer-file-name))

  (set (make-local-variable 'require-final-newline) t)

  (set (make-local-variable 'font-lock-defaults)
       '(liblouis-font-lock-keywords
         nil				; KEYWORDS-ONLY: no
         nil				; CASE-FOLD: no
         ((?_ . "w"))			; SYNTAX-ALIST
	 ))
  (run-hooks 'liblouis-mode-hook))

(provide 'liblouis-mode)

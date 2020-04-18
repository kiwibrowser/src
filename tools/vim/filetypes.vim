" To get syntax highlighting and tab settings for gyp(i), DEPS, and 'git cl'
" changelist description files, add the following to your .vimrc file:
"     so /path/to/src/tools/vim/filetypes.vim
"
augroup filetype
        au! BufRead,BufNewFile *.gyp    set filetype=python expandtab tabstop=2 shiftwidth=2
        au! BufRead,BufNewFile *.gypi   set filetype=python expandtab tabstop=2 shiftwidth=2
        au! BufRead,BufNewFile DEPS     set filetype=python expandtab tabstop=2 shiftwidth=2
        au! BufRead,BufNewFile cl_description* set filetype=gitcommit
augroup END

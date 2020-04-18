// ESLint rules for the Blink streams implementation.
// Based on the devtools rules, with extensive additions and modifications to
// reflect existing usage.

// eslint-disable-next-line no-undef
module.exports = {
  root: true,

  env: {browser: true, es6: true},

  parserOptions: {ecmaVersion: 8},

  // ESLint rules
  //
  // All available rules: http://eslint.org/docs/rules/
  //
  // All rules should have severity "error". For individual exceptions, use
  // comments like:
  // eslint-disable-next-line no-console
  //
  // If a rule is causing problems in multiple places, just remove it.
  rules: {
    // syntax preferences
    quotes: [
      'error', 'single',
      {avoidEscape: true, allowTemplateLiterals: true}
    ],
    semi: 'error',
    'no-extra-semi': 'error',
    'comma-style': ['error', 'last'],
    'wrap-iife': ['error', 'inside'],
    'spaced-comment': 'error',
    eqeqeq: 'error',
    'accessor-pairs':
        ['error', {getWithoutSet: false, setWithoutGet: false}],
    curly: ['error', 'all'],
    'new-parens': 'error',
    'func-call-spacing': 'error',
    'arrow-parens': ['error', 'as-needed'],

    'max-len': ['error', {code: 80, ignoreUrls: true}],

    // Security
    strict: ['error', 'function'],

    // no
    'no-alert': 'error',
    'no-caller': 'error',
    'no-cond-assign': 'error',
    'no-console': 'error',
    'no-debugger': 'error',
    'no-dupe-args': 'error',
    'no-dupe-keys': 'error',
    'no-duplicate-case': 'error',
    'no-else-return': 'error',
    'no-empty': 'error',
    'no-empty-character-class': 'error',
    'no-empty-pattern': 'error',
    'no-eq-null': 'error',
    'no-ex-assign': 'error',
    'no-extra-boolean-cast': 'error',
    'no-extra-parens': [
      'error', 'all', {
        conditionalAssign: false,
        nestedBinaryExpressions: false,
        returnAssign: false
      }
    ],
    'no-eval': 'error',
    'no-fallthrough': 'error',
    'no-floating-decimal': 'error',
    'no-func-assign': 'error',
    'no-implied-eval': 'error',
    'no-implicit-coercion': 'error',
    'no-implicit-globals': 'error',
    'no-inner-declarations': 'off',
    'no-invalid-regexp': 'error',
    'no-irregular-whitespace': 'error',
    'no-labels': 'error',
    'no-loop-func': 'off',
    'no-magic-numbers': 'off',
    'no-multi-str': 'error',
    'no-negated-in-lhs': 'error',
    'no-new-func': 'error',
    'no-new-wrappers': 'error',
    'no-new-object': 'error',
    'no-octal': 'error',
    'no-octal-escape': 'error',
    'no-self-compare': 'error',
    'no-sequences': 'error',
    'no-shadow-restricted-names': 'error',
    'no-sparse-arrays': 'error',
    'no-undef': 'error',
    'no-unexpected-multiline': 'error',
    'no-unsafe-finally': 'error',
    'no-unreachable': 'error',
    'no-unsafe-negation': 'error',
    'no-unused-vars': 'error',
    'no-useless-call': 'error',
    'no-useless-concat': 'error',
    'no-useless-escape': 'error',
    'no-void': 'error',
    'no-with': 'error',
    radix: 'error',
    'use-isnan': 'error',
    'valid-typeof': 'error',

    // ES6
    'no-const-assign': 'error',
    'no-dupe-class-members': 'error',
    'no-new-symbol': 'error',
    'no-this-before-super': 'error',
    'no-var': 'error',
    'prefer-const': 'error',
    'prefer-rest-params': 'error',
    'prefer-spread': 'error',
    'template-curly-spacing': ['error', 'never'],

    // spacing details
    'space-infix-ops': 'error',
    'space-in-parens': ['error', 'never'],
    'no-whitespace-before-property': 'error',
    'keyword-spacing': [
      'error', {
        overrides: {
          if: {after: true},
          else: {after: true},
          for: {after: true},
          while: {after: true},
          do: {after: true},
          switch: {after: true},
          return: {after: true}
        }
      }
    ],
    'arrow-spacing': ['error', {after: true, before: true}],
    'one-var': ['error', 'never'],
    'operator-assignment': ['error', 'always'],
    'operator-linebreak': ['error', 'after'],
    'padded-blocks': ['error', 'never'],
    'space-before-blocks': ['error', 'always'],
    'space-before-function-paren': ['error', 'never'],
    'space-unary-ops': ['error', {words: true, nonwords: false}],

    // file whitespace
    'no-multiple-empty-lines': 'error',
    'no-mixed-spaces-and-tabs': 'error',
    'no-trailing-spaces': 'error',
    'linebreak-style': ['error', 'unix'],

    indent: ['error', 2, {SwitchCase: 1}],

    'brace-style': ['error', '1tbs', {allowSingleLine: true}],

    'key-spacing': [
      'error', {beforeColon: false, afterColon: true, mode: 'strict'}
    ],

    'quote-props': ['error', 'as-needed'],

    'unicode-bom': ['error', 'never']
  }
};

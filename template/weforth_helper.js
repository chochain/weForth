///
/// @file
/// @brief - weForth Javascript helper
///
///===============================================================================================
/// Forth vocabulary lookup table
///
var _forth_voc = {
    '!'       : [ 'mm', '( n a -- )',   'Store n in variable at a' ],
    "'"       : [ 'cm', '( " name" -- xt )', 'Return token of next idiom' ],
    '"'       : [ 'li', '( -- w )',     'Compile input between quotes (") as a string literal to TOS' ],
    '('       : [ 'li', '( " ccc<paren>" -- )', 'Treat text between (...) as comment' ],
    '*'       : [ 'au', '( a b -- c )', 'Multiply NOS and TOS' ],
    '*/'      : [ 'au', '( a b c -- d )',   'd = a * b / c' ],
    '*/mod'   : [ 'au', '( a b c -- d e )', 'd = (a*b) % c, e = (a*b)/c' ],
    '+'       : [ 'au', '( a b -- c )', 'Add TOS to NOS' ],
    '+!'      : [ 'mm', '( n w -- )',   'Add n to the variable at w' ],
    '+loop'   : [ 'br', 'todo', 'todo' ],
    ','       : [ 'cm', '( n -- )',     'Compile number n to pf field of newest word' ],
    '-'       : [ 'au', '( a b -- c )', 'Subtract TOS from NOS' ],
    '.'       : [ 'io', '( a -- )',     'Display number or string a on TOS' ],
    '."'      : [ 'li', '( -- )',       'display input between quotes (") as a string literal' ],
    '.('      : [ 'li', '( -- )',       'Display next idiom up to next )' ],
    '.r'      : [ 'io', '( a n -- )',   'Display a in n columns' ],
    '.s'      : [ 'db', '( -- )',       'Display data stack content' ],
    '/'       : [ 'au', '( a b -- c )', 'Divide NOS by TOS' ],
    '0<'      : [ 'eq', '( a -- f )',   'Return true if a<0' ],
    '0='      : [ 'eq', '( a -- f )',   'Return true if a=0' ],
    '0>'      : [ 'eq', '( a -- f )',   'Return true if a>0' ],
    '0branch' : [ 'br', '( f -- )',     'Branch to the following address if TOS is 0' ],
    '1+'      : [ 'au', '( n -- n+1 )', 'Add 1 to TOS' ],
    '1-'      : [ 'au', '( n -- n-1 )', 'Minus 1 from TOS' ],
    '2*'      : [ 'au', '( n -- n*2 )', 'TOS multiply by 2' ],
    '2/'      : [ 'au', '( n -- n/2 )', 'TOS divided by 2' ],
    '2drop'   : [ 'ss', '( a b -- )',   'Discard TOS and NOS' ],
    '2dup'    : [ 'ss', '( a b -- a b a b )',             'Duplicate TOS and NOS' ],
    '2over'   : [ 'ss', '( a b c d -- a b c d a b )',     'Duplicate second pair TOS items' ],
    '2swap'   : [ 'ss', '( a b c d -- c d a b )',         'Swap two pairs of TOS items' ],
    '4dup'    : [ 'ss', '( a b c d -- a b c d a b c d )', 'Duplicate top quad TOS items' ],
    ':'       : [ 'cm', '( -- )',       'Define a new colon word' ],
    ';'       : [ 'cm', '( -- )',       'Terminate a colon word' ],
    '<'       : [ 'eq', '( a b -- f )', 'Return true if a<b' ],
    '<='      : [ 'eq', '( a b -- f )', 'Return true if a<=b' ],
    '<>'      : [ 'eq', '( a b -- f )', 'Return true if a is not equal to b' ],
    '='       : [ 'eq', '( a b -- f )', 'Return true if a=b' ],
    '>'       : [ 'eq', '( a b -- f )', 'Return true if a>b' ],
    '>='      : [ 'eq', '( a b -- f )', 'Return true if a>=b' ],
    '>r'      : [ 'br', '( a -- )',     'Push TOS to return stack' ],
    '?'       : [ 'mm', '( w -- )',     'Display contents in variable w' ],
    '?do'     : [ 'br', 'todo', 'todo' ],
    '?dup'    : [ 'ss', '( w -- w w)',  'Duplicate w if not zero' ],
    '@'       : [ 'mm', '( w -- a )',   'Return contents of a variable' ],
    'abort'   : [ 'db', '( -- )',       'Clear rs, ss' ],
    'abs'     : [ 'au', '( a -- b )',   'Return absolute of TOS' ],
    'aft'     : [ 'br', '( -- )',       'Skip loop once for the first time' ],
    'again'   : [ 'br', '( -- )',       'Repeat the begin loop' ],
    'allot'   : [ 'cm', '( n -- )',     'Allocate n item to the current array. Init values are 0' ],
    'and'     : [ 'au', '( a b -- c )', 'Bitwise AND of two TOS items' ],
    'array!'  : [ 'mm', '( n w i -- )', 'Store n into ith item of array w' ],
    'array@'  : [ 'mm', '( w i -- b )', 'Return contents of the ith item in array w' ],
    'base'    : [ 'io', '( -- a )',     'Return current base address' ],
    'begin'   : [ 'br', '( -- )',       'Start a begin loop structure' ],
    'bl'      : [ 'io', '( -- )',       'Send a blank char to console' ],
    'branch'  : [ 'br', '( -- )',       'Branch to following address' ],
    'boot'    : [ 'os', '( -- )',       'Trim dictionary back to the fence' ],
    'bye'     : [ 'os', '( -- )',       'exit to OS' ],
    'constant': [ 'cm', '( a -- )',     'Create a new constant with value a' ],
    'cr'      : [ 'io', '( -- )',       'Display a carriage return' ],
    'create'  : [ 'cm', '( -- )',       'Create a new array' ],
    'date'    : [ 'os', '( -- )',       'Display data-time string' ],
    'decimal' : [ 'io', '( -- )',       'Change to decimal base' ],
    'delay'   : [ 'os', '( n -- )',     'Wait n milliseconds' ],
    'depth'   : [ 'db', '( -- n )',     'Number of elements on data stack' ],
    'do'      : [ 'br', '( m n -- )',   'Repeat loop from n to m' ],
    'does>'   : [ 'cm', '( -- )',       'Assign following token list to the new word just created' ],
    'dolit'   : [ 'li', '( -- )',       'Push next token on stack' ],
    'donext'  : [ 'br', '( -- )',       'Loop to the following address' ],
    'dostr'   : [ 'li', '( -- w )',     'Return token of next string' ],
    'dotstr'  : [ 'li', '( -- )',       'Display next compiled string' ],
    'drop'    : [ 'ss', '( a -- )',     'Discard TOS' ],
    'dict'    : [ 'db', '( -- )',       'display woll words' ],
    'dump'    : [ 'db', '( i n -- )',   'Display words in dictionary from index a with n count' ],
    'dup'     : [ 'ss', '( a -- a a )', 'Duplicate TOS' ],
    'else'    : [ 'br', '( -- )',       'Take the next false branch' ],
    'emit'    : [ 'io', '( a -- )',     'Display an ASCII character' ],
    'execute' : [ 'cm', '( -- )',       'Process parsed token' ],
    'exec'    : [ 'cm', '( -- )',       'Process parsed token' ],
    'exit'    : [ 'os', '( -- )',       'Unnest a list' ],
    'eval'    : [ 'os', '( -- )',       'Javascript.eval string on TOS (dangerous, use with care)' ],
    'find'    : [ 'db', '( -- w )',     'Return token of next idiom' ],
    'for'     : [ 'br', '( n -- )',     'Repeat following loop n+1 times' ],
    'forget'  : [ 'db', '( -- )',       'Trim dictionary back to the following idiom' ],
    'here'    : [ 'db', '( -- w )',     'Return top of dictionary' ],
    'hex'     : [ 'io', '( -- )',       'Change to hexadecimal base' ],
    'i'       : [ 'br', '( -- a )',     'Duplicate top of return stack to TOS' ],
    'if'      : [ 'br', '( f -- )',     'Skip the next true branch if TOS is 0' ],
    'immediate':[ 'cm', '( -- )',       'set colon word just defined as immediate word' ],
    'included': [ 'os', '( s n -- )',   'load external file with char string s with length n' ],
    'int'     : [ 'au', '( a -- n )',   'Change TOS to integer' ],
    'invert'  : [ 'au', '( n -- n )',   'Invert all binary digit' ],
    'is'      : [ 'cm', '( w -- )',     'Force next word to execute w. interpret only' ],
    'j'       : [ 'br', '( -- n )',     'Fetch 2nd on return stack' ],
    'key'     : [ 'io', '( -- n)',      'Fetch one keystoke from input terminal' ],
    'loop'    : [ 'br', '( -- )',       'Incement top of return stack. Exit loop if >= than 2nd top of return stack' ],
    'lshift'  : [ 'au', '( n i -- n )', 'Shift n by i bits' ],
    'max'     : [ 'au', '( a b -- c )', 'Return larger of two TOS items' ],
    'min'     : [ 'au', '( a b -- c )', 'Return smaller of two TOS items' ],
    'mod'     : [ 'au', '( a b -- c )', 'Modulus NOS by TOS' ],
    '/mod'    : [ 'au', '( a b -- r q )', 'a divided by b, r=a%b, q=a/b' ],
    '*/mod'   : [ 'au', '( a b c-- r q )','double d= a * b, r=r%c, q=d/b' ],
    'ms'      : [ 'os', '( n -- )',     'Delay n milliseconds' ],
    'mstat'   : [ 'db', '( -- )',       'Display memory statistics' ],
    'n!'      : [ 'mm', '( n w i -- )', 'Store n into ith item of array w' ],
    'n@'      : [ 'mm', '( w i -- b )', 'Return contents of the ith item in array w' ],
    'negate'  : [ 'au', '( a -- b )',   'Negate TOS' ],
    'next'    : [ 'br', '( -- )',       'Decrement top of return stack. Exit loop if top of return stack is negative' ],
    'nip'     : [ 'ss', '( a b -- a )', 'Discard NOS' ],
    'or'      : [ 'au', '( a b -- c )', 'Bitwise OR of two TOS items' ],
    'over'    : [ 'ss', '( a b -- a b a )', 'Duplicate NOS' ],
    'parse'   : [ 'cm', '( -- )',       'Prase out next token in input buffer' ],
    'peek'    : [ 'mm', '( a -- n )',   'Fetch content in memory address a' ],
    'poke'    : [ 'mm', '( a n -- )',   'Store value n into memory address a' ],
    'pick'    : [ 'ss', '( i -- a )',   'Copy ith TOS item to top' ],
    'quit'    : [ 'os', '( -- )',       'Outer interpreter' ],
    'r>'      : [ 'br', '( -- a )',     'Pop return stack to TOS' ],
    'r@'      : [ 'br', '( -- a )',     'Duplicate top of return stack to TOS' ],
    'repeat'  : [ 'br', '( -- )',       'Repeat begin loop' ],
    'roll'    : [ 'ss', '( i -- a )',   'Roll ith TOS item to top' ],
    'rot'     : [ 'ss', '( a b c -- b c a )', 'Rotata 3rd TOS item to top' ],
    '-rot'    : [ 'ss', '( a b c -- c a b )', 'Rotate TOS to 3rd position' ],
    'rshift'  : [ 'au', '( n i -- n )', 'Right shift n by i bits' ],
    's"'      : [ 'li', '( -- w )',     'Compile next idiom as a string literal' ],
    'see'     : [ 'db', '( -- )',       'Disassemble the following idiom' ],
    'space'   : [ 'io', '( -- )',       'Display a space' ],
    'spaces'  : [ 'io', '( n -- )',     'Display n spaces' ],
    'swap'    : [ 'ss', '( a b -- b a )', 'Swap two TOS items' ],
    'then'    : [ 'br', '( -- )',       'Terminate an if-else-then branch structure' ],
    'time'    : [ 'os', '( -- )',       'Display system time string' ],
    'to'      : [ 'cm', '( n -- )',     'Change the value of next constant token to n. compile only' ],
    'type'    : [ 'io', '( a n -- )',   'Send character string at address a with length to console' ],
    'u.'      : [ 'io', '( n -- )',     'Display value n as unsigned' ],
    'u.r'     : [ 'io', '( a n -- )',   'Display unsinged a in n columns' ],
    'u<'      : [ 'au', '( a b -- f )', 'Check condition unsigned a < unsigned b' ],
    'u>'      : [ 'au', '( a b -- f )', 'Check condition unsigned a > unsigned b' ],
    'ucase!'  : [ 'io', '( n -- )',     'Set input case sensitivity n=0 not sensitive' ],
    'until'   : [ 'br', '( f -- )',     'Repeat begin loop if TOS  is 0' ],
    'variable': [ 'cm', '( -- )',       'Create a new variable with initial value of 0' ],
    'while'   : [ 'br', '( f -- )',     'Skip the following true branch if TOS is 0' ],
    'words'   : [ 'db', '( -- )',       'Display names of all words in dictionary' ],
    'xor'     : [ 'au', '( a b -- c )', 'Bitwise XOR of two TOS items' ],
    '['       : [ 'cm', '( -- )',       'Change to interpreting mode' ],
    '\\'      : [ 'li', '( -- )',       'Comment to the end of line' ],
    ']'       : [ 'cm', '( -- )',       'Change to compiling mode' ],
    'acos'    : [ 'ex', '( a -- b )',   'Return arc cosine of TOS' ],
    'asin'    : [ 'ex', '( a -- b )',   'Return arc sine of TOS' ],
    'atan2'   : [ 'ex', '( a b -- c )', 'Return arc tangent of a/b' ],
    'ceil'    : [ 'ex', '( a -- n )',   'Ceiling TOS to integer' ],
    'cos '    : [ 'ex',  '( a -- b )',  'Return cosine of TOS' ],
    'exp'     : [ 'ex', '( a -- b )',   'Return exponential of TOS' ],
    'log'     : [ 'ex', '( a -- b )',   'Return logarithmic of TOS' ],
    'floor'   : [ 'ex', '( a -- n )',   'Floor TOS to integer' ],
    'pi'      : [ 'ex', '( -- pi )',    'Return PI' ],
    'pow'     : [ 'ex', '( a b -- c )', 'Return a to the b-th power' ],
    'random'  : [ 'ex', '( -- a )',     'Return a random number betwee 0 and 1' ],
    'sin'     : [ 'ex', '( a -- b )',   'Return sine of TOS' ],
    'sqrt'    : [ 'ex', '( a -- b )',   'Return square root of TOS' ],
    'tan'     : [ 'ex', '( a -- b )',   'Return tangent of TOS' ],
    'CS'      : [ 'gr', '( -- )',       'Clear Graphic surface' ],
    'HT'      : [ 'gr', '( -- )',       'Hide Turtle' ],
    'ST'      : [ 'gr', '( -- )',       'Show Turtle' ],
    'CT'      : [ 'gr', '( -- )',       'Center Turtle' ],
    'PD'      : [ 'gr', '( -- )',       'Pen Down' ],
    'PU'      : [ 'gr', '( -- )',       'Pen Up' ],
    'HD'      : [ 'gr', '( a -- )',     'Heading in degree' ],
    'FD'      : [ 'gr', '( n -- )',     'Forward n pixel' ],
    'BK'      : [ 'gr', '( n -- )',     'Backward n pixel' ],
    'RT'      : [ 'gr', '( a -- )',     'Right Turn a degree' ],
    'LT'      : [ 'gr', '( a -- )',     'Left Turn a degree' ],
    'PC'      : [ 'gr', '( r g b -- )', 'Pen Color' ],
    'PW'      : [ 'gr', '( n -- )',     'Pen Width in pixel' ],
    'XY'      : [ 'gr', '( x y -- )',   'Pen Position at (x,y)' ],
    'JS'      : [ 'gr', '( s n -- )',   'Execute Javascript char string with length' ]
}
///
/// Vocabulary helpers
///
function _esc(e) {
    return e && e
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;")
}
function _voc(name) {
    return _forth_voc[name] || _forth_voc[name.substring(1)] || null
}
function _category(name) {
    const _cat = {                            ///< category desc lookup
        ss: 'Stack',  au: 'ALU',    eq: 'Compare', io: 'IO',    li: 'Literal',
        br: 'Branch', mm: 'Memory', cm: 'Compile', db: 'Debug', os: 'OS',
        ex: 'Math Ext.', gr: 'Graphic'
    }
    return _voc(name) && _cat[_voc(name)[0]]
}
function _tooltip(name) {
    let voc = _voc(name)
    return voc && '<li><a href="#">' +
        `<div class="tip">${_esc(name)}<i class="tiptip">` +
        `${name.toUpperCase()} ${_esc(voc[1])} ${_esc(voc[2])}` +
        '</i></div></a></li>'
}
function _see(w, n=0) {
    let div = ''
    const _show_pf = (pf)=>{
        if (pf == null || pf.length == 0) return
        div += '[ '
        pf.forEach(w1=>{ div += _see(w1, n+1) })   /// * recursive call
        div += '] '                                /// * close section
    }
    div += w.name+' '                              /// * display word name
    if (w.qf != null && w.qf.length > 0) {         /// * display qf array
        div += '='+JSON.stringify(w.qf)+' '
    }
    _show_pf(w.pf)                             /// * if.{pf}, for.{pf}, or begin.{pf}
    _show_pf(w.pf1)                            /// * else.{pf1}.then, or .then.{pf1}.next
    _show_pf(w.pf2)                            /// * aft.{pf2}.next
    return div
}
function voc_tree(nlst) {
    const voc = nlst.reduce((r,n)=>{
        const c = _category(n)                 ///< get category
        if (c) {
            if (r[c]) r[c].push(n); else r[c] = [ n ]
        }
        return r
    }, {})
    let div = ''
    Object.keys(voc).sort().forEach(k=>{
        div += `<ul class="tree"><li><a href="#">${_esc(k)}</a><ul>`
        voc[k].forEach(n=>{ div += _tooltip(n) })
        div += '</ul></li></ul>'
    })
    return div
}
function colon_words(nlst) {
    let div = '<ul class="tree"><li class="open"><a href="#">User</a><ul>'
    for (let i = nlst.length - 1;
         i >= 0 && nlst[i] != 'boot'; --i) {
        let xt = nlst[i] // todo: _see(nlst[i])
        div += '<li><a href="#"><div class="tip">' +
            `${_esc(nlst[i])}<i>${xt}</i>` +
            '</div></a></li>'
    }
    return div+'</li></ul>'
}

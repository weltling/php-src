--TEST--
Test mkdir/rmdir eucjp to UTF-8 path 
--SKIPIF--
<?php
include dirname(__FILE__) . DIRECTORY_SEPARATOR . "util.inc";

skip_if_no_required_exts();
skip_if_not_win();

?>
--FILE--
<?php
/*
#vim: set fileencoding=eucjp
#vim: set encoding=eucjp
*/

include dirname(__FILE__) . DIRECTORY_SEPARATOR . "util.inc";

$path = dirname(__FILE__) . DIRECTORY_SEPARATOR . "data" . DIRECTORY_SEPARATOR . "テストマルチバイト・パス42"; // EUCJP string
$pathw = iconv('eucjp', 'utf-8', $path);

$subpath = $path . DIRECTORY_SEPARATOR . "テストマルチバイト・パス4"; // EUCJP string
$subpathw = iconv('eucjp', 'utf-8', $subpath);

/* The mb dirname exists*/
var_dump(file_exists($pathw));

var_dump(mkdir($subpathw));
var_dump(file_exists($subpathw));

get_basename_with_cp($subpathw, 65001);

var_dump(rmdir($subpathw));

?>
===DONE===
--EXPECTF--	
bool(true)
bool(true)
bool(true)
Active code page: 65001
getting basename of %s\�����鴻����������������ゃ����祉�����42\�����鴻����������������ゃ����祉�����4
string(0) ""
bool(false)
string(%d) "%s\�����鴻����������������ゃ����祉�����42\�����鴻����������������ゃ����祉�����4"
Active code page: %d
bool(true)
===DONE===

--TEST--
imagecreatefrompng() and empty/missing file
--SKIPIF--
<?php if (!function_exists("imagecreatefrompng")) print "skip"; ?>
--INI--
default_charset=cp1251
--FILE--
<?php

$file = dirname(__FILE__)."/001АБВГДЕЖЗИЙКЛМНОПF.test";
@unlink($file);

var_dump(imagecreatefrompng($file));
touch($file);
var_dump(imagecreatefrompng($file));

@unlink($file);

echo "Done\n";
?>
--EXPECTF--	
Warning: imagecreatefrompng(%s001АБВГДЕЖЗИЙКЛМНОПF.test): failed to open stream: No such file or directory in %s on line %d
bool(false)

Warning: imagecreatefrompng(): '%s001АБВГДЕЖЗИЙКЛМНОПF.test' is not a valid PNG file in %s on line %d
bool(false)
Done

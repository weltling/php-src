--TEST--
Phar: tar-based phar corrupted
--SKIPIF--
<?php if (!extension_loaded('phar')) die('skip'); ?>
<?php if (!extension_loaded("spl")) die("skip SPL not available"); ?>
--INI--
default_charset=cp1251
--FILE--
<?php
include dirname(__FILE__) . '/files/make_invalid_tar.php.inc';

$tar = new corrupter(dirname(__FILE__) . '/tar_001АБВГДЕЖЗИЙКЛМНОПF.phar.tar', 'none');
$tar->init();
$tar->addFile('tar_001АБВГДЕЖЗИЙКЛМНОПF.phpt', __FILE__);
$tar->close();

$tar = fopen('phar://' . dirname(__FILE__) . '/tar_001АБВГДЕЖЗИЙКЛМНОПF.phar.tar/tar_001АБВГДЕЖЗИЙКЛМНОПF.phpt', 'rb');
try {
	$phar = new Phar(dirname(__FILE__) . '/tar_001АБВГДЕЖЗИЙКЛМНОПF.phar.tar');
	echo "should not execute\n";
} catch (Exception $e) {
	echo $e->getMessage() . "\n";
}
?>
===DONE===
--CLEAN--
<?php
@unlink(dirname(__FILE__) . '/tar_001.phar.tar');
?>
--EXPECTF--
Warning: fopen(phar://%star_001АБВГДЕЖЗИЙКЛМНОПF.phar.tar/tar_001АБВГДЕЖЗИЙКЛМНОПF.phpt): failed to open stream: phar error: "%star_001АБВГДЕЖЗИЙКЛМНОПF.phar.tar" is a corrupted tar file (truncated) in %star_001АБВГДЕЖЗИЙКЛМНОПF.php on line 9
phar error: "%star_001АБВГДЕЖЗИЙКЛМНОПF.phar.tar" is a corrupted tar file (truncated)
===DONE===

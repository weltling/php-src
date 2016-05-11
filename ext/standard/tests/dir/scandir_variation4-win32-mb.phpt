--TEST--
Test scandir() function : usage variations - different relative paths
--FILE--
<?php
/* Prototype  : array scandir(string $dir [, int $sorting_order [, resource $context]])
 * Description: List files & directories inside the specified path 
 * Source code: ext/standard/dir.c
 */

/*
 * Test scandir() with relative paths as $dir argument
 */

echo "*** Testing scandir() : usage variations ***\n";

// include for create_files/delete_files functions
include (dirname(__FILE__) . '/../私はガラスを食べられますfile/私はガラスを食べられますfile.inc');

$base_dir_path = dirname(__FILE__);

$level_one_dir_path = "$base_dir_path/私はガラスを食べられますlevel_one";
$level_two_dir_path = "$level_one_dir_path/私はガラスを食べられますlevel_two";

// create directories and files
mkdir($level_one_dir_path);
create_files($level_one_dir_path, 2, 'numeric', 0755, 1, 'w', '私はガラスを食べられますlevel_one', 1);
mkdir($level_two_dir_path);
create_files($level_two_dir_path, 2, 'numeric', 0755, 1, 'w', '私はガラスを食べられますlevel_two', 1);

echo "\n-- \$path = './私はガラスを食べられますlevel_one': --\n";
var_dump(chdir($base_dir_path));
var_dump(scandir('./私はガラスを食べられますlevel_one'));

echo "\n-- \$path = 'level_one/私はガラスを食べられますlevel_two': --\n";
var_dump(chdir($base_dir_path));
var_dump(scandir('私はガラスを食べられますlevel_one/私はガラスを食べられますlevel_two'));

echo "\n-- \$path = '..': --\n";
var_dump(chdir($level_two_dir_path));
var_dump(scandir('..'));

echo "\n-- \$path = '私はガラスを食べられますlevel_two', '.': --\n";
var_dump(chdir($level_two_dir_path));
var_dump(scandir('.'));

echo "\n-- \$path = '../': --\n";
var_dump(chdir($level_two_dir_path));
var_dump(scandir('../'));

echo "\n-- \$path = './': --\n";
var_dump(chdir($level_two_dir_path));
var_dump(scandir('./'));

echo "\n-- \$path = '../../'私はガラスを食べられますlevel_one': --\n";
var_dump(chdir($level_two_dir_path));
var_dump(scandir('../../私はガラスを食べられますlevel_one'));

@delete_files($level_one_dir_path, 2, '私はガラスを食べられますlevel_one');
@delete_files($level_two_dir_path, 2, '私はガラスを食べられますlevel_two');
?>
===DONE===
--CLEAN--
<?php
$dir_path = dirname(__FILE__);
rmdir("$dir_path/私はガラスを食べられますlevel_one/私はガラスを食べられますlevel_two");
rmdir("$dir_path/私はガラスを食べられますlevel_one");
?>
--EXPECTF--
*** Testing scandir() : usage variations ***

-- $path = './私はガラスを食べられますlevel_one': --
bool(true)
array(5) {
  [0]=>
  string(1) "."
  [1]=>
  string(2) ".."
  [2]=>
  string(14) "私はガラスを食べられますlevel_one1.tmp"
  [3]=>
  string(14) "私はガラスを食べられますlevel_one2.tmp"
  [4]=>
  string(9) "私はガラスを食べられますlevel_two"
}

-- $path = 'level_one/level_two': --
bool(true)
array(4) {
  [0]=>
  string(1) "."
  [1]=>
  string(2) ".."
  [2]=>
  string(14) "私はガラスを食べられますlevel_two1.tmp"
  [3]=>
  string(14) "私はガラスを食べられますlevel_two2.tmp"
}

-- $path = '..': --
bool(true)
array(5) {
  [0]=>
  string(1) "."
  [1]=>
  string(2) ".."
  [2]=>
  string(14) "私はガラスを食べられますlevel_one1.tmp"
  [3]=>
  string(14) "私はガラスを食べられますlevel_one2.tmp"
  [4]=>
  string(9) "私はガラスを食べられますlevel_two"
}

-- $path = 'level_two', '.': --
bool(true)
array(4) {
  [0]=>
  string(1) "."
  [1]=>
  string(2) ".."
  [2]=>
  string(14) "私はガラスを食べられますlevel_two1.tmp"
  [3]=>
  string(14) "私はガラスを食べられますlevel_two2.tmp"
}

-- $path = '../': --
bool(true)
array(5) {
  [0]=>
  string(1) "."
  [1]=>
  string(2) ".."
  [2]=>
  string(14) "私はガラスを食べられますlevel_one1.tmp"
  [3]=>
  string(14) "私はガラスを食べられますlevel_one2.tmp"
  [4]=>
  string(9) "私はガラスを食べられますlevel_two"
}

-- $path = './': --
bool(true)
array(4) {
  [0]=>
  string(1) "."
  [1]=>
  string(2) ".."
  [2]=>
  string(14) "私はガラスを食べられますlevel_two1.tmp"
  [3]=>
  string(14) "私はガラスを食べられますlevel_two2.tmp"
}

-- $path = '../../'level_one': --
bool(true)
array(5) {
  [0]=>
  string(1) "."
  [1]=>
  string(2) ".."
  [2]=>
  string(14) "私はガラスを食べられますlevel_one1.tmp"
  [3]=>
  string(14) "私はガラスを食べられますlevel_one2.tmp"
  [4]=>
  string(9) "私はガラスを食べられますlevel_two"
}
===DONE===

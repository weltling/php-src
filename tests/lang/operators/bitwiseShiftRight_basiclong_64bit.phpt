--TEST--
Test >> operator : 64bit long and bigint tests
--FILE--
<?php
 
define("MAX_64Bit", 9223372036854775807);
define("MAX_32Bit", 2147483647);
define("MIN_64Bit", -9223372036854775807 - 1);
define("MIN_32Bit", -2147483647 - 1);

$longVals = array(
    MAX_64Bit, MIN_64Bit, MAX_32Bit, MIN_32Bit, MAX_64Bit - MAX_32Bit, MIN_64Bit - MIN_32Bit,
    MAX_32Bit + 1, MIN_32Bit - 1, MAX_32Bit * 2, (MAX_32Bit * 2) + 1, (MAX_32Bit * 2) - 1, 
    MAX_64Bit -1, MAX_64Bit + 1, MIN_64Bit + 1, MIN_64Bit - 1
);

$otherVals = array(0, 1, 7, 9, 65);

error_reporting(E_ERROR);

foreach ($longVals as $longVal) {
   foreach($otherVals as $otherVal) {
	   echo "--- testing: $longVal >> $otherVal ---\n";   
      var_dump($longVal>>$otherVal);
   }
}
   
?>
===DONE===
--EXPECT--
--- testing: 9223372036854775807 >> 0 ---
int(9223372036854775807)
--- testing: 9223372036854775807 >> 1 ---
int(4611686018427387903)
--- testing: 9223372036854775807 >> 7 ---
int(72057594037927935)
--- testing: 9223372036854775807 >> 9 ---
int(18014398509481983)
--- testing: 9223372036854775807 >> 65 ---
int(0)
--- testing: -9223372036854775808 >> 0 ---
int(-9223372036854775808)
--- testing: -9223372036854775808 >> 1 ---
int(-4611686018427387904)
--- testing: -9223372036854775808 >> 7 ---
int(-72057594037927936)
--- testing: -9223372036854775808 >> 9 ---
int(-18014398509481984)
--- testing: -9223372036854775808 >> 65 ---
int(-1)
--- testing: 2147483647 >> 0 ---
int(2147483647)
--- testing: 2147483647 >> 1 ---
int(1073741823)
--- testing: 2147483647 >> 7 ---
int(16777215)
--- testing: 2147483647 >> 9 ---
int(4194303)
--- testing: 2147483647 >> 65 ---
int(0)
--- testing: -2147483648 >> 0 ---
int(-2147483648)
--- testing: -2147483648 >> 1 ---
int(-1073741824)
--- testing: -2147483648 >> 7 ---
int(-16777216)
--- testing: -2147483648 >> 9 ---
int(-4194304)
--- testing: -2147483648 >> 65 ---
int(-1)
--- testing: 9223372034707292160 >> 0 ---
int(9223372034707292160)
--- testing: 9223372034707292160 >> 1 ---
int(4611686017353646080)
--- testing: 9223372034707292160 >> 7 ---
int(72057594021150720)
--- testing: 9223372034707292160 >> 9 ---
int(18014398505287680)
--- testing: 9223372034707292160 >> 65 ---
int(0)
--- testing: -9223372034707292160 >> 0 ---
int(-9223372034707292160)
--- testing: -9223372034707292160 >> 1 ---
int(-4611686017353646080)
--- testing: -9223372034707292160 >> 7 ---
int(-72057594021150720)
--- testing: -9223372034707292160 >> 9 ---
int(-18014398505287680)
--- testing: -9223372034707292160 >> 65 ---
int(-1)
--- testing: 2147483648 >> 0 ---
int(2147483648)
--- testing: 2147483648 >> 1 ---
int(1073741824)
--- testing: 2147483648 >> 7 ---
int(16777216)
--- testing: 2147483648 >> 9 ---
int(4194304)
--- testing: 2147483648 >> 65 ---
int(0)
--- testing: -2147483649 >> 0 ---
int(-2147483649)
--- testing: -2147483649 >> 1 ---
int(-1073741825)
--- testing: -2147483649 >> 7 ---
int(-16777217)
--- testing: -2147483649 >> 9 ---
int(-4194305)
--- testing: -2147483649 >> 65 ---
int(-1)
--- testing: 4294967294 >> 0 ---
int(4294967294)
--- testing: 4294967294 >> 1 ---
int(2147483647)
--- testing: 4294967294 >> 7 ---
int(33554431)
--- testing: 4294967294 >> 9 ---
int(8388607)
--- testing: 4294967294 >> 65 ---
int(0)
--- testing: 4294967295 >> 0 ---
int(4294967295)
--- testing: 4294967295 >> 1 ---
int(2147483647)
--- testing: 4294967295 >> 7 ---
int(33554431)
--- testing: 4294967295 >> 9 ---
int(8388607)
--- testing: 4294967295 >> 65 ---
int(0)
--- testing: 4294967293 >> 0 ---
int(4294967293)
--- testing: 4294967293 >> 1 ---
int(2147483646)
--- testing: 4294967293 >> 7 ---
int(33554431)
--- testing: 4294967293 >> 9 ---
int(8388607)
--- testing: 4294967293 >> 65 ---
int(0)
--- testing: 9223372036854775806 >> 0 ---
int(9223372036854775806)
--- testing: 9223372036854775806 >> 1 ---
int(4611686018427387903)
--- testing: 9223372036854775806 >> 7 ---
int(72057594037927935)
--- testing: 9223372036854775806 >> 9 ---
int(18014398509481983)
--- testing: 9223372036854775806 >> 65 ---
int(0)
--- testing: 9223372036854775808 >> 0 ---
int(9223372036854775808)
--- testing: 9223372036854775808 >> 1 ---
int(4611686018427387904)
--- testing: 9223372036854775808 >> 7 ---
int(72057594037927936)
--- testing: 9223372036854775808 >> 9 ---
int(18014398509481984)
--- testing: 9223372036854775808 >> 65 ---
int(0)
--- testing: -9223372036854775807 >> 0 ---
int(-9223372036854775807)
--- testing: -9223372036854775807 >> 1 ---
int(-4611686018427387904)
--- testing: -9223372036854775807 >> 7 ---
int(-72057594037927936)
--- testing: -9223372036854775807 >> 9 ---
int(-18014398509481984)
--- testing: -9223372036854775807 >> 65 ---
int(-1)
--- testing: -9223372036854775809 >> 0 ---
int(-9223372036854775809)
--- testing: -9223372036854775809 >> 1 ---
int(-4611686018427387905)
--- testing: -9223372036854775809 >> 7 ---
int(-72057594037927937)
--- testing: -9223372036854775809 >> 9 ---
int(-18014398509481985)
--- testing: -9223372036854775809 >> 65 ---
int(-1)
===DONE===

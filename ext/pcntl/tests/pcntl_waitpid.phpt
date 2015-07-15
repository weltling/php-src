--TEST--
pcntl_waitpid functionality
--SKIPIF--
<?php
	if (!extension_loaded('pcntl')) echo 'skip';
?>
--FILE--
<?php
$pid = pcntl_spawn();

function test_exit_waits(){
	print "\n\nTesting pcntl_wifexited and wexitstatus....";

	$pid=pcntl_fork();
	if ($pid==0) {
		sleep(1);
		exit(-1);
	} else {
		$options=0;
		pcntl_waitpid($pid, $status, $options);
		if ( pcntl_wifexited($status) ) print "\nExited With: ". pcntl_wexitstatus($status);
	}
}


print "Staring wait.h tests....";
test_exit_waits();
test_exit_signal();
test_stop_signal();
?>
--EXPECT--
Staring wait.h tests....

Testing pcntl_wifexited and wexitstatus....
Exited With: 255

Testing pcntl_wifsignaled....
Process was terminated by signal : SIGTERM

Testing pcntl_wifstopped and pcntl_wstopsig....
Process was stoped by signal : SIGSTOP
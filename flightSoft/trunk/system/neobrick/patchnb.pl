#!/usr/bin/perl

use Net::Telnet;
use Net::FTP;

$neobrick = '10.0.0.2';
$reboot = 1;

$telnet = new Net::Telnet (
	Timeout => 5, 
	Errmode =>'return');

$ret = $telnet->open($neobrick);
if (!$ret) {
  print "Unable to connect: ".$telnet->errmsg."\n";
  exit 2;
}
$ret = $telnet->waitfor('/#\ /');
if (!$ret) {
  print "Login prompt not seen: ".$telnet->errmsg."\n";
  exit 2;
}
print "Login successful\n";
$telnet->cmd('grep -v xferlog_enable /etc/vsftpd.conf > /etc/vsftpd.conf.tmp');

#
# Strip out "xferlog_enable = YES"
#
#
@checklines = $telnet->cmd('busybox md5sum /etc/vsftpd.conf.tmp');
@checkstrings = split(/ /, $checklines[0]);
if ($checkstrings[0] cmp "5b1c968fc242cdede58a00f2d79cfa79") {
	print "Error patching /etc/vsftpd.conf...!";
	if ($reboot) {
	  $telnet->cmd('reboot');
	}
	$telnet->close;
	exit 1;
}
print "Patching /etc/vsftpd.conf ... step 1 okay\n";

#
# Add in "xferlog_enable=NO"
#
$telnet->cmd('echo xferlog_enable=NO >> /etc/vsftpd.conf.tmp');
@checklines = $telnet->cmd('busybox md5sum /etc/vsftpd.conf.tmp');
@checkstrings = split(/ /, $checklines[0]);
if ($checkstrings[0] cmp "654ae417a97f545af3e49634f3c9b878") {
	print "Error patching /etc/vsftpd.conf...!";
	if ($reboot) {
	  $telnet->cmd('reboot');
	}
	$telnet->close;
	exit 1;
}
print "Patching /etc/vsftpd.conf ... step 2 okay\n";

#
# Replace /etc/vsftpd.conf
#
$telnet->cmd('cp /etc/vsftpd.conf.tmp /etc/vsftpd.conf');
@checklines = $telnet->cmd('busybox md5sum /etc/vsftpd.conf');
@checkstrings = split(/ /, $checklines[0]);
if ($checkstrings[0] cmp "654ae417a97f545af3e49634f3c9b878") {
	print "Error patching /etc/vsftpd.conf...!";
	if ($reboot) {
	  $telnet->cmd('reboot');
	}
	$telnet->close;
	exit 1;
}
print "Patching /etc/vsftpd.conf ... step 3 okay\n";

#
# SIGHUP the running vsftpd process
#
$telnet->cmd('killall -SIGHUP vsftpd');

print "VSFTPD patching complete.\n";

$telnet->cmd('rm -rf /mnt/data/neobrickd');

$ftp = Net::FTP->new($neobrick, Timeout => 5);
$ftp->login("anonymous");
$ftp->binary;
$myerr = $ftp->put("neobrickd","data/neobrickd");
if (!$myerr) {
	print "FTP transfer failed!";
	$ftp->close;
	if ($reboot) {
	  $telnet->cmd('reboot');
	}
	$telnet->close;	
	exit 1;
}
$ftp->close;
print "neobrickd transfer complete.\n";


@checklines = $telnet->cmd('busybox md5sum /mnt/data/neobrickd');
@checkstrings = split(/ /, $checklines[0]);
if ($checkstrings[0] cmp "5008eeaba2e1ce2b5cbfe8abdbc657a9") {
	print "FTP transfer failed (md5sum)!";
	if ($reboot) {
	  $telnet->cmd('reboot');
	}
	$telnet->close;
	exit 1;
}
print "neobrickd verification complete.\n";

$telnet->cmd('killall neobrickd');
$telnet->cmd('cp /mnt/data/neobrickd /usr/bin/neobrickd');
@checklines = $telnet->cmd('busybox md5sum /usr/bin/neobrickd');
@checkstrings = split(/ /, $checklines[0]);
if ($checkstrings[0] cmp "5008eeaba2e1ce2b5cbfe8abdbc657a9") {
	print "Copy failed (md5sum)?!";
	if ($reboot) {
	  $telnet->cmd('reboot');
	}
	$telnet->close;
	exit 1;
}
$telnet->cmd('/usr/bin/neobrickd < /dev/null > /dev/null 2>&1 &');
print "neobrickd patching complete.\n";
print "All patching complete.\n";

$telnet->close;
exit 0;

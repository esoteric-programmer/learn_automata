<?php

$files = scandir('.');
$nfa = array();
$afa = array();
$dfas = array();
$canceled = 0;
$nontrivial = 0;
$trivial = 0;
foreach($files as $file) {
	if (preg_match('/^seed_[0-9]+\\.log$/',$file)!==1) {
		continue;
	}
	if (filesize($file) <= 0) {
		continue;
	}
	$f = fopen($file,'r');
	$seed_content = fread($f,filesize($file));
	fclose($f);
	if (preg_match('/AL\\*\\* output after ([0-9]+) EQ and ([0-9]+) MQ:/', $seed_content, $al2)===1) {
		preg_match('/AL\\* output after ([0-9]+) EQ and ([0-9]+) MQ:/', $seed_content, $al);
		preg_match('/DFA size: ([0-9]+) state(s?)/', $seed_content, $dfa_size);
		preg_match('/NFA size: ([0-9]+) state(s?)/', $seed_content, $nfa_size);
		preg_match('/AFA size: ([0-9]+) state(s?)/', $seed_content, $afa_size);
		preg_match('/RAFA size: ([0-9]+) state(s?)/', $seed_content, $afa2_size);

		if ($afa2_size[1] <= 0 || $afa_size[1] <= 0) {
			$trivial++;
			continue;
		}

		if (!isset($nfa[$dfa_size[1]+0])) {
			$nfa[$dfa_size[1]+0] = array();
		}
		$nfa[$dfa_size[1]+0][] = $nfa_size[1];

		if (!isset($afa[$dfa_size[1]+0])) {
			$afa[$dfa_size[1]+0] = array();
		}
		$afa[$dfa_size[1]+0][] = $afa2_size[1];
		if (!isset($dfas[$dfa_size[1]+0])) {
			$dfas[$dfa_size[1]+0] = 0;
		}
		$dfas[$dfa_size[1]+0]++;

		$nontrivial++;


/*
		if ($al2[1] > $al[1] + 1) {
			print $file.' - EQ: '.$al[1].' -> '.$al2[1].'; MQ: '.$al[2].' -> '.$al2[2].'; DFA: '.$dfa_size[1].', NFA: '.$nfa_size[1].', AFA1:'.$afa_size[1].', AFA2:'.$afa2_size[1]."\n";
		}else if ($afa_size[1] > 1){
			print 'DFA: '.$dfa_size[1].', NFA: '.$nfa_size[1].', AFA1:'.$afa_size[1].', AFA2:'.$afa2_size[1]."\n";
		}
*/

	}else{
		$canceled++;
	}
}
$num_fa = array();
foreach ($nfa as $dfa_size => $nfa_sizes) {
	$sum = 0;
	$cnt = 0;
	foreach ($nfa_sizes as $nfa_size) {
		$sum += $nfa_size;
		$cnt++;
	}
	$sum /= $cnt;
	$nfa[$dfa_size] = $sum;
	$num_fa[$dfa_size] = $cnt;
}
foreach ($afa as $dfa_size => $afa_sizes) {
	$sum = 0;
	$cnt = 0;
	foreach ($afa_sizes as $afa_size) {
		$sum += $afa_size;
		$cnt++;
	}
	$sum /= $cnt;
	$afa[$dfa_size] = $sum;
}
ksort($nfa);
ksort($afa);
ksort($dfas);
ksort($num_fa);
/*
foreach ($dfas as $dfa_size => $num_dfas) {
	if (!isset($d[floor($dfa_size / 10)])) {
		$d[floor($dfa_size / 10)] = 0;
	}
	$d[floor($dfa_size / 10)] += $num_dfas;
}

print 'Frequencies:'."\n";
foreach ($d as $dfa_size => $num_dfas) {
	print 'DFA size '.($dfa_size*10).'-'.($dfa_size*10+9).': '.$num_dfas."\n";
}
print "\n";
*/
print 'DFA NFA AFA num'."\n";
print '0 0 0 0'."\n";
$i = 1;
foreach ($nfa as $dfa_size => $nfa_size) {
print $dfa_size.' '.$nfa_size.' '.$afa[$dfa_size].' '.$num_fa[$dfa_size]."\n";
$i++;
}
print 'number of trivial instances: '.$trivial.' of '.($canceled + $nontrivial + $trivial).' ('.(round(1000.0*$trivial/($canceled+$trivial+$nontrivial))/10.0).'%).'."\n";
print 'canceled: '.$canceled.' of '.($canceled + $nontrivial).' non-trivial instances ('.(round(1000.0*$canceled/($canceled+$nontrivial))/10.0).'%).'."\n";
?>

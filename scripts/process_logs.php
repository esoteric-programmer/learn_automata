<?php

$files = scandir('.');
$nfa = array();
$afa = array();
$dfas = array();
$canceled = 0;
$nontrivial = 0;
$trivial = 0;
$eq_growth = 0;
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
		preg_match('/NL\\* output after ([0-9]+) EQ and ([0-9]+) MQ:/', $seed_content, $nl);
		preg_match('/\\nL\\* output after ([0-9]+) EQ and ([0-9]+) MQ:/', $seed_content, $l);
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



		
		if ($al2[1] > $al[1] + 1) {
			$eq_growth++;
		}
		/*else if ($afa_size[1] > 1){
			print 'DFA: '.$dfa_size[1].', NFA: '.$nfa_size[1].', AFA1:'.$afa_size[1].', AFA2:'.$afa2_size[1]."\n";
		}*/
		print $file.":\n";
		print "\t".'DFA: '.$dfa_size[1].', NFA: '.$nfa_size[1].', AFA1: '.$afa_size[1].', AFA2: '.$afa2_size[1]."\n";
		print "\t".'EQ: '.$l[1].' -> '.$nl[1].' -> '.$al[1].' -> '.$al2[1].'; MQ: '.$l[2].' -> '.$nl[2].' -> '.$al[2].' -> '.$al2[2]."\n";


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
print "\n";
print 'DFA NFA AFA'."\n";
print '0 0 0'."\n";
$fa_smoothed = array();
$fa_smoothed['dfa'] = 0;
$fa_smoothed['nfa'] = 0;
$fa_smoothed['afa'] = 0;
$fa_smoothed['num'] = 0;
$last_reset = 0;
$i = 0;
foreach ($nfa as $dfa_size => $nfa_size) {
$i++;
$fa_smoothed['dfa'] += $dfa_size * $num_fa[$dfa_size];
$fa_smoothed['nfa'] += $nfa_size * $num_fa[$dfa_size];
$fa_smoothed['afa'] += $afa[$dfa_size] * $num_fa[$dfa_size];
$fa_smoothed['num'] += $num_fa[$dfa_size];
if ($fa_smoothed['num'] >= 9 && ($last_reset+($dfa_size/12) < $i)) {
print ($fa_smoothed['dfa'] / $fa_smoothed['num']).' '.($fa_smoothed['nfa'] / $fa_smoothed['num']).' '.($fa_smoothed['afa'] / $fa_smoothed['num'])."\n";
$fa_smoothed['dfa'] = 0;
$fa_smoothed['nfa'] = 0;
$fa_smoothed['afa'] = 0;
$fa_smoothed['num'] = 0;
$last_reset = $i;
}
}
print "\n";
print 'number of trivial instances: '.$trivial.' of '.($canceled + $nontrivial + $trivial).' ('.(round(1000.0*$trivial/($canceled+$trivial+$nontrivial))/10.0).'%).'."\n";
print 'canceled: '.$canceled.' of '.($canceled + $nontrivial).' non-trivial instances ('.(round(1000.0*$canceled/($canceled+$nontrivial))/10.0).'%).'."\n";
print 'number of non-trivial instances with more than one additional EQ query: '.$eq_growth.' of '.($nontrivial).' ('.(round(1000.0*$eq_growth/($nontrivial))/10.0).'%)'."\n";
?>

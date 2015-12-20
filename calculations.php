<?php

$fs = 6000; // sampling frequency

function bpf($f0, $q, $fs) {
    $w0 = 2 * M_PI * $f0 / $fs;
    $cos0 = cos($w0);
    $sin0 = sin($w0);
    $alpha = $sin0 / (2 * $q);

    $b0 = $alpha;
    $b1 = 0;
    $b2 = - $alpha;
    $a0 = 1 + $alpha;
    $a1 = -2 * $cos0;
    $a2 = 1 - $alpha;
    $c0 = 65536 * 4;
    $c = $c0 / $a0;
    $b0 = round($b0 * $c);
    $b1 = round($b1 * $c);
    $b2 = round($b2 * $c);
    $a0 = $c0;
    $a1 = round($a1 * $c);
    $a2 = round($a2 * $c);
    return array('a' => array($a0, $a1, $a2), 'b' => array($b0, $b1, $b2));
}

function generate($f, $time, $fs) {
    $res = array();
    $amp = 120;
    $offs = 128;
    $w = 2 * M_PI * $f;
    $dt = 1 / $fs;
    for ($t = 0; $t < $time; $t += $dt) {
        $res[] = floor(sin($t * $w) * $amp + $offs);
    }
    return $res;
}

function amplitude($data) {
    $avg = 0;
    $n = count($data);
    foreach ($data as $x) {
        $avg += $x;
    }
    $avg /= $n;
    $avg2 = 0;
    foreach ($data as $x) {
        $avg2 += pow($x - $avg, 2);
    }
    return sqrt($avg2 / $n);
}

function filter($data, $filter) {
    list($a0, $a1, $a2) = $filter['a'];
    list($b0, $b1, $b2) = $filter['b'];
    $res = array($data[0], $data[1]);
    for ($i = 2; $i < count($data); $i++) {
        $y = ($b0 * $data[$i] + $b1 * $data[$i - 1] + $b2 * $data[$i - 2]
                - $a1 * $res[$i - 1] - $a2 * $res[$i - 2]);
        $res[] = floor($y / $a0);
    }
    return $res;
}

function test($freq, $prn = false) {
    global $fs, $filter;
    $data = generate($freq, 0.1, $fs);
    $res = filter($data, $filter);
    if ($prn) {
        for ($i = 0; $i < count($data); $i++) {
            echo "{$data[$i]}\t{$res[$i]}\n";
        }
    }
    $aIn = amplitude($data);
    $aOut = amplitude($res);
    return $aOut / $aIn;
}

$filter = bpf(1109, 13, $fs);
echo "Coeffs A: " . implode(', ', $filter['a']) . "\n";
echo "Coeffs B: " . implode(', ', $filter['b']) . "\n";

echo "C5: " . test(523) . "\n";
echo "D5: " . test(587) . "\n";
echo "E5: " . test(659) . "\n";
echo "F5: " . test(698) . "\n";
echo "G5: " . test(784) . "\n";
echo "A5: " . test(880) . "\n";
echo "B5: " . test(988) . "\n";
echo "C6: " . test(1047) . "\n";
echo "D6: " . test(1175) . "\n";
echo "E6: " . test(1319) . "\n";
echo "F6: " . test(1397) . "\n";
echo "G6: " . test(1568) . "\n";


#!/usr/bin/perl -w
####################################################################################################
###                                            bch.pl                                            ###
####################################################################################################
###                                                                                              ###
###                                                                                              ###
### Author: u/ufioes                                                                             ###
####################################################################################################

use strict;

BEGIN {

	use Getopt::Long;
	use Pod::Usage;
	use Data::Dumper;

	my $path = $0;
	$path =~ s'/[^/]*$'';

}

my $template_svg = <<'EOF';
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg>
%1$s
</svg>
EOF

my $template_column = <<'EOF';
<g transform="translate(%2$f, -%1$f) rotate(45) scale(0.707)">
%3$s
</g>
EOF

my $template_path = <<'EOF';
<g
inkscape:label="Layer 1"
inkscape:groupmode="layer"
id="layer1">
<path
style="fill:none;stroke:#000000;stroke-width:1;"
d="m %1$s"
/>
</g>
EOF

####################################################################################################
###											 VARIABLES                                           ###
####################################################################################################

my $data = [];
#Phonetic version
#my $nodes = [
#	[6488, "cUp"],
#	[6350, "fIt"],
#	[2011, "cAt"],
#	[1324, "lEd"],
#	[1265, "dAte"],
#	[1174 + 667, "cAlm"],
#	[1083, "fEEt"],
#	[987, "nOOn"],
#	[971, "nOte"],
#	[954, "tIme"],
#	[648, "fOOt"],
#	[414, "hOUse"],
#	[42, "OIl"],
#	[5179, "No"],
#	[4945, "Tea"],
#	[4625, "Red"],
#	[3186, "See"],
#	[2377, "Let"],
#	[2180, "THe"],
#	[2091, "Day"],
#	[1924, "Kill"],
#	[1870, "May"],
#	[1539, "Zoo"],
#	[1518, "Voice"],
#	[1468, "Pay"],
#	[1151 + 241, "Wall"],
#	[1075, "Bay"],
#	[1051, "Fly"],
#	[780, "Yes"],
#	[745, "Go"],
#	[721, "Hot"],
#	[565, "SHe"],
#	[522, "siNG"],
#	[343, "CHeck"],
#	[325, "Jar"],
#	[286, "THing"],
#	[19, "pleaSure"],
#];

#alphabatical letter frequencies
my $nodes = [
	[0.08167, "a"],
	[0.01492, "b"],
	[0.02782, "c"],
	[0.04253, "d"],
	[0.12702, "e"],
	[0.02228, "f"],
	[0.02015, "g"],
	[0.06094, "h"],
	[0.06966, "i"],
	[0.00153, "j"],
	[0.00772, "k"],
	[0.04025, "l"],
	[0.02406, "m"],
	[0.06749, "n"],
	[0.07507, "o"],
	[0.01929, "p"],
	[0.00095, "q"],
	[0.05987, "r"],
	[0.06327, "s"],
	[0.09056, "t"],
	[0.02758, "u"],
	[0.00978, "v"],
	[0.02360, "w"],
	[0.00150, "x"],
	[0.01974, "y"],
	[0.00074, "z"],
];
my $encoding = {};

my $dump_mode;
my $flow_input;
my $in_filename;

####################################################################################################
###                                        MAIN FUNCTIONS                                        ###
####################################################################################################

main();

exit 0;

sub main {

	GetOptions("help|h|?" => sub {pod2usage(1)}, "man" => sub {pod2usage("-verbose" => 2)}, 
		"dump|d" => \$dump_mode, "flow|f=s" => \$flow_input, "i=s" => \$in_filename) or pod2usage(2);

	#generate Huffman tree.
	#TODO make the tree always contain " " as some string of 0s
	while (scalar(@{$nodes}) > 1) {
		$nodes = [sort {$a->[0] <=> $b->[0]} @{$nodes}];
		my $a = shift(@{$nodes});
		my $b = shift(@{$nodes});
		push(@{$nodes}, [$a->[0] + $b->[0], $a, $b]);
	}

	#create text to Huffman encoding.
	$encoding = {reverse split(/\s/, print_tree($nodes->[0]))};

	if ($dump_mode) {
		print print_tree($nodes->[0]);
		return;
	}

	#TODO add support for combining multiple lines in one file???
	if (defined $in_filename and -e -r -f $in_filename) {

		open(my $input, "<", $in_filename) or die "Could not open $in_filename\n";

		my $lines = [];

		while (<$input>) {
			chomp;

			my $file_input = $_;

			foreach my $line (grep {length $_} split(/[.?!]+|\n+/, $file_input)) {

				my $words = [];

				foreach my $word (grep {length $_} split(/ +/, $line)) {
					my $square = flow($word);
					my $grid = gridify($square);
					my ($directions, $size) = directionalize($grid);
					push(@{$words}, [$directions, $size]);
				}

				push(@{$lines}, $words);

			}

		}

		print_svg($lines);

		return;

	} elsif (defined $flow_input) {

		my $lines = [];

		foreach my $line (grep {length $_} split(/[.?!]+|\n+/, $flow_input)) {

			my $words = [];

			foreach my $word (grep {length $_} split(/ +/, $line)) {
				my $square = flow($word);
				my $grid = gridify($square);
				my ($directions, $size) = directionalize($grid);
				push(@{$words}, [$directions, $size]);
			}

			push(@{$lines}, $words);

		}

		print_svg($lines);

		#legacy code!
		#my $offset = 0;
		#foreach my $square (flow($flow_input)) {
		#	my $output = [];
		#	my $size = 0;
		#	foreach my $pos (sort keys %{$square}) {
		#		$size = $size > $_ ? $size : $_ foreach map {int} split(/ /, $pos);
		#	}
		#	$size++;
		#	for (my $x = 0; $x < $size; $x++) {
		#		for (my $y = 0; $y < $size; $y++) {
		#			$output->[$x]->[$y] = ".";
		#		}
		#	}
		#	foreach (sort keys %{$square}) {
		#		my $val = $square->{$_};
		#		my $pos = [split(/ /, $_)];
		#		$output->[$pos->[0]]->[$pos->[1]] = $val == 2 ? "#" : "*";
		#	}
		#	for (my $x = 0; $x < $size; $x++) {
		#		print " " x $offset;
		#		for (my $y = 0; $y < $size; $y++) {
		#			print $output->[$x]->[$y];
		#		}
		#		print "\n";
		#	}
		#	$offset += $size;
		#}
		return;
	}

	return;

}

sub flow {

	my $input = lc shift;

	my $seq;

	if ($input =~ /^[01]+$/) {
		$seq = [split(//, $input)];
	} elsif ($input =~ /^[a-z]+$/) {
		$seq = [map {split(//, $encoding->{$_})} split(//, $input)];
	} else {
		die "Input only [a-zA-Z] please.\n";
	}

	my $size = 1;

	my $success = 0;
	my $next = 0;
	my $adjacent = {};

	my $awd = 0;

	while (1) {

		$adjacent = {};
		$next = 0;

		for (my $i = 0; $i < $size; $i++) {
			$adjacent->{"$i 0"} = 1;
			$adjacent->{($size - 1) . " $i"} = 1;
		}

		while ($next < scalar(@{$seq}) and scalar(grep {$_ == 1} values %{$adjacent})) {

			foreach my $pos (sort {priority($a, $b)} keys %{$adjacent}) {
				if ($adjacent->{$pos} == 1) {

					#test legality
					$adjacent->{$pos} = 2;
					unless (legal($adjacent, $size)) {
						$adjacent->{$pos} = -2;
						last;
					}

					$adjacent->{$pos} = 1;

					if ($seq->[$next++]) {
						$adjacent->{$_} //= 1 foreach (@{neighborhood($pos, $size)});
						$adjacent->{$pos} = 2;
						last;
					} else {
						$adjacent->{$pos} = -1;
						last;
					}
				} else {
					next;
				}
			}

		}

		foreach my $pos (grep {$adjacent->{$_} == 1} keys %{$adjacent}) {

			#test legality THIS LOOKS WRONG!
			#$adjacent->{$pos} = 2;
			#unless (legal($adjacent, $size)) {
			$adjacent->{$pos} = -2;
			#}

		}

		#if ($success == -1) {
		#	my $re = [];
		#	for (my $i = $next; $i < scalar(@{$seq}); $i++) {
		#		push(@{$re}, $seq->[$i]);
		#	}
		#	return ($adjacent, flow(substr(join("", @{$seq}), $next, scalar(@{$seq}) - $next)));
		#} elsif (not scalar(grep {$_ == 1} values %{$adjacent})) {
		my $remaining = scalar(@{$seq}) - $next;
		if ($remaining == 0) {
			$success = 1;
			return $adjacent;
		} else {
			$size++;
		}
		#} else {
		#	warn "I don't think this should happen anymore! Let me know if it does!";
		#	$success = -1;
		#	$size--;
		#	#last;
		#}

	}

}

sub print_svg {

	my $lines = shift;

	my $columns = "";

	my $offset = 0;

	foreach my $words (@{$lines}) {

		my $size = 5;

		my $pad = [0, 0];

		my $paths = "";

		foreach my $word (@{$words}) {
			my $coords = sprintf("%1\$d %1\$d 0 -5 -5 0 0 5 5 0\n", $size + 5);
			$size += 5 * $word->[1] - 5;
			my $pos = 0;
			foreach my $dir (@{$word->[0]}) {
				$coords .= sprintf("%d %d\n", $dir->[1] * 5, $dir->[0] * 5);
				$pos += $dir->[1] - $dir->[0];
				$pad->[0] = $pos if $pos < $pad->[0];
				$pad->[1] = $pos if $pos > $pad->[1];
			}
			$coords .= "0 5 5 0 0 -5 -5 0";
			$paths .= sprintf($template_path, $coords);
		}

		$size += 10;

		$offset += -$pad->[0] * 5 / 2 + 5/2;

		$columns .= sprintf($template_column, $size / 2, $offset, $paths); 

		$offset += $pad->[1] * 5 / 2 + 5/2;

	}

	printf($template_svg, $columns);

}

####################################################################################################
###										  HELPER FUNCTIONS                                       ###
####################################################################################################

sub priority {

	my $a = [split(/ /, shift())];
	my $b = [split(/ /, shift())];

	return ($a->[0] <=> $b->[0] or $a->[1] <=> $b->[1]);

}

sub legal {

	my $adjacent = shift;
	my $size = shift;

	my $grid = {};
	my $good = {};

	for (my $x = 0; $x < $size; $x++) {
		for (my $y = 0; $y < $size; $y++) {
			unless ($adjacent->{$x . " " . $y} and $adjacent->{$x . " " . $y} == 2) {
				$grid->{$x . " " . $y} = 1;
			}
		}
	}

	for (my $i = 0; $i < $size; $i++) {
		if ($grid->{"0 $i"}) {
			$good->{"0 $i"} = 1;
		}
		if ($grid->{"$i " . ($size - 1)}) {
			$good->{"$i " . ($size - 1)} = 1;
		}
	}

	while (grep {$_ == 1} values %{$good}) {
		foreach my $pos (keys %{$good}) {
			if ($good->{$pos} == 1) {
				foreach (@{neighborhood($pos, $size)}) {
					if ($grid->{$_}) {
						$good->{$_} //= 1;
					}
				}
				$good->{$pos} = 2;
				last;
			}
		}
	}

	foreach my $pos (keys %{$grid}) {
		return 0 unless $good->{$pos};
	}

	return 1;

}

sub neighborhood {

	my $pos = [split(/ /, shift())];
	my $size = shift;

	my $output = [];

	my $test = [
		[$pos->[0] + 1, $pos->[1]],
		[$pos->[0] - 1, $pos->[1]],
		[$pos->[0], $pos->[1] + 1],
		[$pos->[0], $pos->[1] - 1],
	];

	foreach my $x (@{$test}) {
		if ($x->[0] >= 0 and $x->[0] < $size and $x->[1] >= 0 and $x->[1] < $size) {
			push(@{$output}, $x->[0] . " " . $x->[1]);
		}
	}

	return $output;

}

sub gridify {

	my $points = shift;

	#[00 01 02]
	#[10 11 12]
	#[20 21 22]
	my $output = [];

	foreach my $pos (keys %{$points}) {
		my ($y, $x) = map {int} split(/ /, $pos);
		$output->[$y + 1]->[$x + 1] = ($points->{$pos} == 2) ? 1 : 0;
	}

	my $size = $#{$output} + 2;

	for (my $y = 0; $y < $size; $y++) {
		$output->[$y]->[0] = 1;
	}

	for (my $x = 0; $x < $size; $x++) {
		$output->[$#{$output}]->[$x] = 1;
	}

	return $output;
}

sub directionalize {

	my $grid = shift;
	my $size = scalar @{$grid};

	my $output = [];

	my $x = 0;
	my $y = 0;

	while ($x < $size - 1 and $y < $size - 1) {

		my $dir;

		if ($grid->[$y + 1]->[$x + 1]) {
			if ($grid->[$y]->[$x + 1]) {
				if ($grid->[$y]->[$x]) {
					$dir = [0, -1];
				} else {
					$dir = [-1, 0];
				}
			} else {
				$dir = [0, 1];
			}
		} else {
			if ($grid->[$y + 1]->[$x]) {
				$dir = [1, 0];
			} else {
				if ($grid->[$y]->[$x]) {
					$dir = [0, -1];
				} else {
					$dir = [-1, 0];
				}
			}
		}

		push(@{$output}, $dir);

		$y += $dir->[0];
		$x += $dir->[1];

	}

	$#{$output}--;

	return $output, $size;

}

sub print_tree {

	my $node = shift;
	my $prefix = shift() // "";

	if ($node->[2]) {
		my $output = "";
		$output .= print_tree($node->[1], $prefix . "0");
		$output .= print_tree($node->[2], $prefix . "1");
		return $output;
	} else {
		return sprintf("%s %s\n", $prefix, $node->[1]);
	}

}



__END__
=pod

=head1 SYNOPSIS

B<bch.pl> [F<OPTIONS>]

=head1 OPTIONS

=over

=item B<-help>

Print usage information and exit.

=item B<-man>

Print a fancy man page and exit.

=item B<-f -flow>

Flow an input sequence / text and draw svg.

=item B<-i>

Flow a file.

=item B<-d -dump>

Print out the generated Huffman tree.

=back

=cut

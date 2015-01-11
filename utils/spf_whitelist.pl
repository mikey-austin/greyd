#!/usr/bin/perl
#
# Read greydb(8) output, parse for GREY entries and
# if an SPF record is found, verify the domain and
# whitelist permitted mail servers.
#

use warnings;
use strict;

use Mail::SPF;

my %domains;
my @whitelist;
my $server = Mail::SPF::Server->new;

while(<>) {
    chomp;
    if(/^GREY\|([^|]+)\|([^|]+)\|([^|]+)\|.+/) {
        my $req = &make_spf_request($1, $2, $3);
        if(not exists $domains{$req->domain}) {
            $domains{$req->domain} = 1;
            my $res = $server->process($req);
            if($res->code eq 'pass') {
                &whitelist_domain($req->domain);
            }
        }
    }
}

# Print addresses to whitelist.
foreach my $w (@whitelist) {
    print "$w\n";
}

sub make_spf_request {
    my ($ip, $helo, $from) = @_;
    my $request = Mail::SPF::Request->new(
        versions      => [1, 2],
        scope         => 'mfrom',
        ip_address    => $ip,
        helo_identity => $helo,
        identity      => $from,
        );

    return $request;
}

sub whitelist_domain {
    my $domain = shift;

    my @domains;
    my $packet = $server->dns_lookup($domain, 'TXT');
    my @records = $server->get_acceptable_records_from_packet(
        $packet, 'TXT', [1, 2], 'mfrom', $domain);

    foreach my $record (@records) {
        foreach my $term ($record->terms) {
            if($term->name eq 'a') {
                my $a_domain = $term->text;
                $a_domain =~ s/^a://;
                my $a_packet = $server->dns_lookup($a_domain, 'A');
                foreach my $rr ($a_packet->answer) {
                    push @whitelist, $rr->address;
                }
            }
            elsif($term->name eq 'ip4') {
                my $ip = $term->text;
                $ip =~ s/^ip4://;
                push @whitelist, $ip;
            }
            elsif($term->name eq 'include') {
                my $include_domain = $term->text;
                $include_domain =~ s/include://;
                push @domains, $include_domain;
            }
        }

        foreach my $mod ($record->global_mods) {
            if($mod->name eq 'redirect') {
                my $mod_domain = $mod->text;
                $mod_domain =~ s/redirect=//;
                push @domains, $mod_domain;
            }
        }
    }

    foreach my $domain (@domains) {
        &whitelist_domain($domain);
    }
}

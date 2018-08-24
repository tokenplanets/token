/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "tkgame.hpp"
#include <time.h>
#include <ctime>

namespace eosio
{

void CTkgame::create(account_name issuer,
                     asset maximum_supply)
{
    require_auth(_self);
	ckx_http();

    auto sym = maximum_supply.symbol;
    eosio_assert(sym.is_valid(), "invalid symbol name");
    eosio_assert(maximum_supply.is_valid(), "invalid supply");
    //printi(maximum_supply.amount);
   
 	eosio_assert( maximum_supply == asset(10000000000000, string_to_symbol(4,"TKCOINB")), "max-supply must be 1 billion TKCOINB and with 4 decision");

    stats statstable(_self, sym.name());
    auto existing = statstable.find(sym.name());
    eosio_assert(existing == statstable.end(), "token with symbol already exists");
    statstable.emplace(_self, [&](auto &s) {
        s.supply.symbol = maximum_supply.symbol;
        s.max_supply = maximum_supply;
        s.issuer = issuer;
    });
}

void CTkgame::issue(account_name to, asset quantity, string memo)
{
    eosio_assert(to != string_to_name(m_contract.c_str()), "cannot issue to tkcoin");
    auto sym = quantity.symbol;
    eosio_assert(sym.is_valid(), "invalid symbol name");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    auto sym_name = sym.name();
    stats statstable(_self, sym_name);
    auto existing = statstable.find(sym_name);
    eosio_assert(existing != statstable.end(), "token with symbol does not exist, create token before issue");
    const auto &st = *existing;

    require_auth(st.issuer);
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must issue positive quantity");

    eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
    eosio_assert(quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    accounts issuename(_self, to);
    auto existing2 = issuename.find(quantity.symbol.name());
    eosio_assert(existing2 == issuename.end(), "the account has been issued");

    statstable.modify(st, 0, [&](auto &s) {
        s.supply += quantity;
    });

    add_balance(st.issuer, quantity, st.issuer);

    if (to != st.issuer)
    {
        SEND_INLINE_ACTION(*this, transfer, {st.issuer, N(active)}, {st.issuer, to, quantity, memo});
    }

	
}

void CTkgame::transfer(account_name from,
                       account_name to,
                       asset quantity,
                       string memo)
{
    eosio_assert(from != to, "cannot transfer to self");
    require_auth(from);
    if (from != string_to_name(m_contract.c_str()))
    {
        eosio_assert(to != string_to_name(m_team.c_str()), "cannot transfer to team");
        eosio_assert(to != string_to_name(m_community.c_str()), "cannot transfer to community");
        eosio_assert(to != string_to_name(m_investor.c_str()), "cannot transfer to investor");
        eosio_assert(to != string_to_name(m_mine.c_str()), "cannot transfer to mine");

        eosio_assert(to != string_to_name(m_contract.c_str()), "cannot transfer to tkcoin");
    }

    eosio_assert(is_account(to), "to account does not exist");
    auto sym = quantity.symbol.name();
    stats statstable(_self, sym);
    const auto &st = statstable.get(sym);

    require_recipient(from);
    require_recipient(to);

    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    sub_balance(from, quantity);
    add_balance(to, quantity, from);
    tktrans_rec tktransrec(_self, _self);
    auto datetime = current_time();
    //printui(datetime);
    tktransrec.emplace(_self, [&](auto &a) {
        a.id = tktransrec.available_primary_key(); // ��������
        a.from = from;
        a.to = to;
        a.balance = quantity;
        a.datetime = datetime;
    });
}


void CTkgame::sub_balance(account_name owner, asset value)
{
    accounts from_acnts(_self, owner);

    const auto &from = from_acnts.get(value.symbol.name(), "no balance object found");
    eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

    if (from.balance.amount == value.amount)
    {
        from_acnts.erase(from);
    }
    else
    {
        from_acnts.modify(from, owner, [&](auto &a) {
            a.balance -= value;
        });
    }
	if(owner == string_to_name(m_contract.c_str()))
		return;
	tkaccount_list acclist(_self,_self);
	auto datetime = current_time();
	const auto &from2 = acclist.get(owner, "account list no object found");
	acclist.modify(from2, owner, [&](auto &a) {
		a.balance -= value;
		a.datetime = datetime;
	});
}

void CTkgame::add_balance(account_name owner, asset value, account_name ram_payer)
{
    accounts to_acnts(_self, owner);
    auto to = to_acnts.find(value.symbol.name());
    if (to == to_acnts.end())
    {
        to_acnts.emplace(ram_payer, [&](auto &a) {
            a.balance = value;
        });
    }
    else
    {
        to_acnts.modify(to, 0, [&](auto &a) {
            a.balance += value;
        });
    }

	tkaccount_list acclist(_self,_self);
	auto to2 = acclist.find(owner);
	auto datetime = current_time();
	if (to2 == acclist.end())
	{
	    if(owner == string_to_name(m_contract.c_str()))
		    return;
		acclist.emplace(ram_payer,[&](auto &a) {
			a.name = owner;
			a.balance = value;
			a.datetime = datetime;
			if(owner == string_to_name(m_team.c_str())
				||owner == string_to_name(m_community.c_str())
				||owner == string_to_name(m_investor.c_str())
				||owner == string_to_name(m_mine.c_str()))
				a.super_symbol = 1;
		});
	}
	else
	{
		acclist.modify(to2, 0, [&](auto &a) {
		    a.balance += value;
		    a.datetime = datetime;
	    });
	}
	
}

void CTkgame::setsuperuser(account_name name)
{
	require_auth(_self);
	eosio_assert(is_account(name), "the account does not exist");
	tkaccount_list acclist(_self,_self);
	auto to = acclist.find(name);
	if(to != acclist.end())
	{
		acclist.modify(to, _self, [&](auto &a) {
		    a.super_symbol=1;
	    });
	}
}

void CTkgame::deletedata()
{
    require_auth(_self);
	char ckxoutput[1024] = "hello";
	print("Begin deletedata \n");
	//ckx_call("https://mainnet.eoscannon.io", 0, "/v1/chain/get_info", 0, ckxoutput, 0);
	ckx_call("http://nodes.get-scatter.com:80", 0, "/v1/chain/get_info", 0, ckxoutput, 0);
	prints(ckxoutput);
	//print("contract output:");
	//prints_l(ckxoutput,1024);
	//print("\n");
	return;
	//print("m_symbol is:",m_symbol.name(),"\n");
	stats statstable(_self, m_symbol.name());
	auto existing = statstable.find(m_symbol.name());
	if(existing != statstable.end())
	{
		statstable.erase(existing);
	}
	accounts sysname1(_self, string_to_name(m_team.c_str()));
	auto e1 = sysname1.find(m_symbol.name());
	if(e1 != sysname1.end())
	{
		sysname1.erase(e1);
	}
	accounts sysname2(_self, string_to_name(m_community.c_str()));
	auto e2 = sysname2.find(m_symbol.name());
	if(e2 != sysname2.end())
	{
		sysname2.erase(e2);
	}
	accounts sysname3(_self, string_to_name(m_investor.c_str()));
	auto e3 = sysname3.find(m_symbol.name());
	if(e3 != sysname3.end())
	{
		sysname3.erase(e3);
	}
	accounts sysname4(_self, string_to_name(m_mine.c_str()));
	auto e4 = sysname4.find(m_symbol.name());
	if(e4 != sysname4.end())
	{
		sysname4.erase(e4);
	}
	//tktrans;
	tktrans_rec tktransrec(_self, _self);
	
	while(1)
	{
	    auto it1 = tktransrec.begin();
		if(it1 == tktransrec.end())
			break;
		tktransrec.erase(it1);
	}
	print("enter deletedata17 !!!!!!!!!\n");
	//tkaccount;
	tkaccount_list acclist(_self,_self);
	while(1)
	{
	    auto it2 = acclist.begin();
		if(it2 == acclist.end())
			break;
		acclist.erase(it2);
	}
}

} // namespace eosio

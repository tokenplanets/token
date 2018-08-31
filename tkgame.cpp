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
	//ckx_http();

    auto sym = maximum_supply.symbol;
    eosio_assert(sym.is_valid(), "invalid symbol name");
    eosio_assert(maximum_supply.is_valid(), "invalid supply");
    //printi(maximum_supply.amount);
   
 	eosio_assert( maximum_supply == asset(10000000000000, string_to_symbol(4,"TKCOIN")), "max-supply must be 1 billion TKCOIN and with 4 decision");

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
    eosio_assert(to != string_to_name(m_contract.c_str()), "cannot issue to tktkcointkcoincoin");
	eosio_assert(to == string_to_name(m_team.c_str()) ||
		to == string_to_name(m_community.c_str()) ||
		to == string_to_name(m_investor.c_str()) ||
		to == string_to_name(m_mine.c_str()),"must issue to tkcointeamxm,or foundationxm, or footingstone, or tkcoinminexm");

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
        eosio_assert(to != string_to_name(m_team.c_str()), "cannot transfer to tkcointeamxm");
        eosio_assert(to != string_to_name(m_community.c_str()), "cannot transfer to foundationxm");
        eosio_assert(to != string_to_name(m_investor.c_str()), "cannot transfer to footingstone");
        eosio_assert(to != string_to_name(m_mine.c_str()), "cannot transfer to tkcoinminexm");

        eosio_assert(to != string_to_name(m_contract.c_str()), "cannot transfer to tkcointkcoin");
    }

    eosio_assert(is_account(to), "to account does not exist");

	if (to == string_to_name(m_unlock.c_str()))
	{
		eosio_assert(from == string_to_name(m_team.c_str()),"transfer to teamunlockxm must from tkcointeamxm");
	}

	if (to == string_to_name(m_remain.c_str()))
	{
	    string str1="rc-";
		int ret = memo.compare(0,2,str1,0,2);
		if(ret != 0)
		{
		    eosio_assert(from == string_to_name(m_community.c_str()) ||
			    from == string_to_name(m_mine.c_str()),"transfer to remainingsum must from foundationxm or tkcoinminexm");
		}
	}

	if (to == string_to_name(m_provision.c_str()))
	{
		eosio_assert(from == string_to_name(m_remain.c_str()),"transfer to tkcprovision must from remainingsum");
	}

	if (to == string_to_name(m_retrieve.c_str()))
	{
		eosio_assert(from == string_to_name(m_remain.c_str()),"transfer to coinretrieve must from remainingsum");
	}

	if (to == string_to_name(m_dig.c_str()))
	{
		eosio_assert(from == string_to_name(m_mine.c_str()),"transfer to authoritydig must from tkcoinminexm");
	}

	/*if (to == string_to_name(m_operate.c_str()))
	{
		eosio_assert(from == string_to_name(m_team.c_str()),"transfer to tkcoperatexm must from tkcointeamxm");
	}*/
	
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

}

} // namespace eosio

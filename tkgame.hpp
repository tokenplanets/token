/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class CTkgame : public contract {
      public:
         CTkgame( account_name self ):contract(self)
		 {
		     m_contract = "tkcoin";
		     m_team = "team";
			 m_community = "community";
			 m_investor = "investor";
			 m_mine = "mine";
			 m_symbol = string_to_symbol(4,"TKCOINB");
		 }
		 
         void create( account_name issuer,
                      asset        maximum_supply);

         void issue( account_name to, asset quantity, string memo );

         void transfer( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo );
      
         void setsuperuser(account_name name);
		 void deletedata();
         inline asset get_supply( symbol_name sym )const;
         
         inline asset get_balance( account_name owner, symbol_name sym )const;

      private:
	  	 //@abi table accounts i64
         struct account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.name(); }
         };
		
         //@abi table stat i64
         struct currency_stat {
            asset          supply;
            asset          max_supply;
            account_name   issuer;

            uint64_t primary_key()const { return supply.symbol.name(); }
         };

         //@abi table tktrans i64
         struct transfer_rec {
		 	uint64_t id;
		 	account_name from;
			account_name to;
			asset balance;
			uint64_t datetime;

			uint64_t primary_key() const { return id; }
         };

		 //@abi table tkaccount i64
		 struct account_list{
		    account_name name;
			asset balance;
			uint8_t super_symbol=0;
			uint64_t datetime;

			uint64_t primary_key() const {return name; }
		 };
		 
		 typedef eosio::multi_index<N(accounts), account> accounts;
         typedef eosio::multi_index<N(stat), currency_stat> stats;
		 typedef eosio::multi_index<N(tktrans), transfer_rec> tktrans_rec;
		 typedef eosio::multi_index<N(tkaccount), account_list> tkaccount_list; 

         void sub_balance( account_name owner, asset value );
         void add_balance( account_name owner, asset value, account_name ram_payer );
		

         string m_contract;
		 string m_team;
		 string m_community;
		 string m_investor;
		 string m_mine;
		 symbol_type m_symbol;
      public:
         struct transfer_args {
            account_name  from;
            account_name  to;
            asset         quantity;
            string        memo;
         };
   };

   asset CTkgame::get_supply( symbol_name sym )const
   {
      stats statstable( _self, sym );
      const auto& st = statstable.get( sym );
      return st.supply;
   }

   asset CTkgame::get_balance( account_name owner, symbol_name sym )const
   {
      accounts accountstable( _self, owner );
      const auto& ac = accountstable.get( sym );
      return ac.balance;
   }

} /// namespace eosio
EOSIO_ABI(eosio::CTkgame, (create)(issue)(transfer)(setsuperuser)(deletedata))


/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2007 InspIRCd Development Team
 * See: http://www.inspircd.org/wiki/index.php/Credits
 *
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#include <stdio.h>
#include <string>
#include "users.h"
#include "channels.h"
#include "modules.h"
#include "inspircd.h"
#include "m_filter.h"

/* $ModDesc: An advanced spam filtering module */
/* $ModDep: m_filter.h */

typedef std::map<std::string,FilterResult*> filter_t;

class ModuleFilter : public FilterBase
{
 
 filter_t filters;

 public:
	ModuleFilter(InspIRCd* Me)
	: FilterBase::FilterBase(Me, "m_filter.so")
	{
		OnRehash(NULL,"");
	}
	
	virtual ~ModuleFilter()
	{
	}

	virtual FilterResult* FilterMatch(const std::string &text)
	{
		std::string text2 = text+" ";
		for (filter_t::iterator index = filters.begin(); index != filters.end(); index++)
		{
			if ((ServerInstance->MatchText(text2,index->first)) || (ServerInstance->MatchText(text,index->first)))
			{
				FilterResult* fr = index->second;
				if (index != filters.begin())
				{
					std::string pat = index->first;
					filters.erase(fr);
					filters.insert(filters.begin(), std::make_pair(pat,fr));
				}
				return fr;
			}
		}
		return NULL;
	}

	virtual bool DeleteFilter(const std::string &freeform)
	{
		if (filters.find(freeform) != filters.end())
		{
			delete (filters.find(freeform))->second;
			filters.erase(filters.find(freeform));
			return true;
		}
		return false;
	}

	virtual std::pair<bool, std::string> AddFilter(const std::string &freeform, const std::string &type, const std::string &reason, long duration)
	{
		if (filters.find(freeform) != filters.end())
		{
			return std::make_pair(false, "Filter already exists");
		}

		FilterResult* x = new FilterResult;
		x->reason = reason;
		x->action = type;
		x->gline_time = duration;
		x->freeform = freeform;
		filters[freeform] = x;

		return std::make_pair(true, "");
	}

	virtual void SyncFilters(Module* proto, void* opaque)
	{
		for (filter_t::iterator n = filters.begin(); n != filters.end(); n++)
		{
			this->SendFilter(proto, opaque, n->second);
		}
	}

	virtual void OnRehash(userrec* user, const std::string &parameter)
	{
		ConfigReader* MyConf = new ConfigReader(ServerInstance);

		for (int index = 0; index < MyConf->Enumerate("keyword"); index++)
		{
			this->DeleteFilter(MyConf->ReadValue("keyword","pattern",index));

			std::string pattern = MyConf->ReadValue("keyword","pattern",index);
			std::string reason = MyConf->ReadValue("keyword","reason",index);
			std::string do_action = MyConf->ReadValue("keyword","action",index);
			long gline_time = ServerInstance->Duration(MyConf->ReadValue("keyword","duration",index).c_str());
			if (do_action == "")
				do_action = "none";
			FilterResult* x = new FilterResult;
			x->reason = reason;
			x->action = do_action;
			x->gline_time = gline_time;
			x->freeform = pattern;
			filters[pattern] = x;
		}
		DELETE(MyConf);
	}

	virtual int OnStats(char symbol, userrec* user, string_list &results)
	{
		if (symbol == 's')
		{
			std::string sn = ServerInstance->Config->ServerName;
			for (filter_t::iterator n = filters.begin(); n != filters.end(); n++)
			{
				results.push_back(sn+" 223 "+user->nick+" :GLOB:"+n->second->freeform+" "+n->second->action+" "+ConvToStr(n->second->gline_time)+" :"+n->second->reason);
			}
		}
		return 0;
	}
};

// stuff down here is the module-factory stuff. For basic modules you can ignore this.

class ModuleFilterFactory : public ModuleFactory
{
 public:
	ModuleFilterFactory()
	{
	}
	
	~ModuleFilterFactory()
	{
	}
	
	virtual Module * CreateModule(InspIRCd* Me)
	{
		return new ModuleFilter(Me);
	}
	
};


extern "C" void * init_module( void )
{
	return new ModuleFilterFactory;
}


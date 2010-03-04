/* 

                          Firewall Builder

                 Copyright (C) 2002 NetCitadel, LLC

  Author:  Vadim Kurland     vadim@vk.crocodile.org

  $Id$

  This program is free software which we release under the GNU General Public
  License. You may redistribute and/or modify this program under the terms
  of that license as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  To get a copy of the GNU General Public License, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


*/

#include "config.h"

#include "PolicyCompiler_cisco.h"

#include "fwbuilder/FWObjectDatabase.h"
#include "fwbuilder/RuleElement.h"
#include "fwbuilder/IPService.h"
#include "fwbuilder/ICMPService.h"
#include "fwbuilder/TCPService.h"
#include "fwbuilder/UDPService.h"
#include "fwbuilder/Network.h"
#include "fwbuilder/Policy.h"
#include "fwbuilder/Interface.h"
#include "fwbuilder/Management.h"
#include "fwbuilder/Resources.h"
#include "fwbuilder/AddressTable.h"
#include "fwbuilder/Cluster.h"

#include <iostream>
#if __GNUC__ > 3 || \
    (__GNUC__ == 3 && (__GNUC_MINOR__ > 2 || (__GNUC_MINOR__ == 2 ) ) ) || \
     _MSC_VER
#  include <streambuf>
#else
#  include <streambuf.h>
#endif
#include <iomanip>
#include <algorithm>
#include <functional>
#include <map>
#include <assert.h>
#include <cctype>

using namespace libfwbuilder;
using namespace fwcompiler;
using namespace std;


/*
 * Call this rule processor after splitIfSrcMatchesFw and
 * splitIfDstMatchesFw to make sure that if firewall or its interface
 * or address is in src or dst, it is the only object there.
 */
bool PolicyCompiler_cisco::setInterfaceAndDirectionBySrc::processNext()
{
    PolicyRule *rule=getNext(); if (rule==NULL) return false;
    Helper helper(compiler);

    list<int> intf_id_list;

    if (rule->getInterfaceId() == -1)
    {
        bool cluster_member = compiler->fw->getOptionsObject()->getBool("cluster_member");
        Cluster *cluster = NULL;
        if (cluster_member)
            cluster = Cluster::cast(
                compiler->dbcopy->findInIndex(compiler->fw->getInt("parent_cluster_id")));

        RuleElementSrc *srcre = rule->getSrc();
        RuleElementDst *dstre = rule->getDst();
        Address *srcobj = compiler->getFirstSrc(rule);

        if (rule->getDirection()==PolicyRule::Both &&
            ! compiler->complexMatch(srcobj, compiler->fw) &&
            ! compiler->complexMatch(srcobj, cluster))
        {
            intf_id_list = helper.findInterfaceByNetzoneOrAll( srcre );
        }

        if (rule->getDirection()==PolicyRule::Inbound)
            intf_id_list = helper.getAllInterfaceIDs();

        for (list<int>::iterator i = intf_id_list.begin();
             i!=intf_id_list.end(); ++i)
        {
            int intf_id = *i;
            Interface *ifs = Interface::cast(rule->getRoot()->findInIndex(intf_id));
            assert(ifs);
            if (ifs->isUnprotected()) continue;   // skip!

            PolicyRule *new_rule = compiler->dbcopy->createPolicyRule();
            compiler->temp_ruleset->add(new_rule);
            new_rule->duplicate(rule);
            new_rule->setInterfaceId(intf_id);
            new_rule->setDirection(PolicyRule::Inbound);
            new_rule->setBool("interface_and_direction_set_from_src",true);
            tmp_queue.push_back(new_rule);
        }
        // If dst does not match firewall, preserve original rule as
        // well to let setInterfaceAndDirectionByDst work on it.
        FWObject *d = dstre->front();
        if (FWReference::cast(d)!=NULL) d = FWReference::cast(d)->getPointer();
        if (!compiler->complexMatch(Address::cast(d), compiler->fw))
            tmp_queue.push_back(rule);
        return true;
    }

    tmp_queue.push_back(rule);
    return true;
}

bool PolicyCompiler_cisco::setInterfaceAndDirectionByDst::processNext()
{
    PolicyRule *rule=getNext(); if (rule==NULL) return false;
    Helper helper(compiler);

    if (rule->getBool("interface_and_direction_set_from_src"))
    {
        tmp_queue.push_back(rule);
        return true;
    }

    list<int> intf_id_list;

    if (rule->getInterfaceId() == -1)
    {
        bool cluster_member = compiler->fw->getOptionsObject()->getBool("cluster_member");
        Cluster *cluster = NULL;
        if (cluster_member)
            cluster = Cluster::cast(
                compiler->dbcopy->findInIndex(compiler->fw->getInt("parent_cluster_id")));

        RuleElementDst *dstre = rule->getDst();
        Address *dstobj = compiler->getFirstDst(rule);

        if (rule->getDirection()==PolicyRule::Both &&
            ! compiler->complexMatch(dstobj, compiler->fw) &&
            ! compiler->complexMatch(dstobj, cluster))
        {
            intf_id_list = helper.findInterfaceByNetzoneOrAll( dstre );
        }

        if (rule->getDirection()==PolicyRule::Outbound)
            intf_id_list = helper.getAllInterfaceIDs();

        for (list<int>::iterator i = intf_id_list.begin();
             i!=intf_id_list.end(); ++i)
        {
            int intf_id = *i;
            Interface *ifs = Interface::cast(rule->getRoot()->findInIndex(intf_id));
            assert(ifs);
            if (ifs->isUnprotected()) continue;   // skip!

            PolicyRule *new_rule = compiler->dbcopy->createPolicyRule();
            compiler->temp_ruleset->add(new_rule);
            new_rule->duplicate(rule);
            new_rule->setInterfaceId(intf_id);
            new_rule->setDirection(PolicyRule::Outbound);
            new_rule->setBool("interface_and_direction_set_from_dst",true);
            tmp_queue.push_back(new_rule);
        }
        return true;
    }
    tmp_queue.push_back(rule);
    return true;
}

bool PolicyCompiler_cisco::setInterfaceAndDirectionIfInterfaceSet::processNext()
{
    PolicyRule *rule=getNext(); if (rule==NULL) return false;

    //RuleElementItf *itfre=rule->getItf(); 

    if (rule->getInterfaceId() == -1 ||
        rule->getBool("interface_and_direction_set_from_src") ||
        rule->getBool("interface_and_direction_set_from_dst"))
    {
        tmp_queue.push_back(rule);
        return true;
    }

    PolicyRule *new_rule;
                    
    if ( rule->getInterfaceId() > -1 )
    {
        int rule_iface_id = rule->getInterfaceId();

        if (rule->getDirection()==PolicyRule::Both)
        {
            new_rule =compiler->dbcopy->createPolicyRule();
            compiler->temp_ruleset->add(new_rule);
            new_rule->duplicate(rule);
            new_rule->setInterfaceId( rule_iface_id );
            new_rule->setDirection(PolicyRule::Inbound);
            new_rule->setBool("interface_and_direction_set",true);
            tmp_queue.push_back(new_rule);

            new_rule =compiler->dbcopy->createPolicyRule();
            compiler->temp_ruleset->add(new_rule);
            new_rule->duplicate(rule);
            new_rule->setInterfaceId( rule_iface_id );
            new_rule->setDirection(PolicyRule::Outbound);
            new_rule->setBool("interface_and_direction_set",true);
            tmp_queue.push_back(new_rule);
        } else
        {
            new_rule =compiler->dbcopy->createPolicyRule();
            compiler->temp_ruleset->add(new_rule);
            new_rule->duplicate(rule);
            new_rule->setInterfaceId( rule_iface_id );
            // direction is copied from the original rule
            new_rule->setBool("interface_and_direction_set",true);
            tmp_queue.push_back(new_rule);
        }
    }
    return true;
}

bool PolicyCompiler_cisco::pickACL::processNext()
{
    PolicyCompiler_cisco *cisco_comp=dynamic_cast<PolicyCompiler_cisco*>(
        compiler);
    PolicyRule *rule=getNext(); if (rule==NULL) return false;
    
    Interface  *rule_iface = Interface::cast(compiler->dbcopy->findInIndex(
                                                 rule->getInterfaceId()));
    if(rule_iface==NULL)
    {
        compiler->abort(rule, "Missing interface assignment");
        return true;
    }

    /*
     * option 'Generate outbound access lists' is called
     * 'pix_generate_out_acl' for PIX and 'generate_out_acl' for
     * IOS. Need to check the right one depending on what platform
     * this compiler is for. Class PolicyCompiler_cisco is base class
     * and can be used for both.
     */

    bool generate_out_acl = false;

    if (compiler->myPlatformName()=="pix")
        generate_out_acl = compiler->fw->getOptionsObject()->
            getBool("pix_generate_out_acl");
    else
    {
        if (compiler->myPlatformName()=="iosacl")
            generate_out_acl = true;
        else
            generate_out_acl = compiler->fw->getOptionsObject()->
                getBool("generate_out_acl");
    }

    if (rule->getDirection() == PolicyRule::Outbound && !generate_out_acl)
    {
        compiler->abort(
                rule, 
                "Rule with direction 'Outbound' requires outbound ACL "
                "but option 'Generate outbound access lists' is OFF.");
        return true;
    }

    /* The choice of the ACL name depends on whether this is a named
     * acl or not. If not, should use unique numbers. Also need to
     * pass this flag to the ciscoACL object.
     */
    string acl_name = rule_iface->getLabel();
    if (acl_name.empty()) acl_name = rule_iface->getName();
    acl_name = cisco_comp->mangleInterfaceName(acl_name);
    string dir = "in";
    if (rule->getDirection() == PolicyRule::Inbound)
    {
        acl_name += "_in"; dir="in";
    }
    if (rule->getDirection() == PolicyRule::Outbound)
    {
        acl_name += "_out"; dir="out";
    }

    // if this is not the "main" rule set, use its name for the acl name
    if (!compiler->getSourceRuleSet()->isTop())
        acl_name = compiler->getSourceRuleSet()->getName() + "_" + acl_name;

    if (cisco_comp->ipv6) acl_name = "ipv6_" + acl_name;

    rule->setStr("acl",acl_name);

    ciscoACL *acl = new ciscoACL(acl_name, rule_iface, dir, using_named_acl);
    cisco_comp->acls[acl_name] = acl;

    acl->setWorkName(acl_name);

    tmp_queue.push_back(rule);
    return true;
}

/*
 * Take interface name as an argument and produce
 * shortened, space-free string that uniquely identifies interface
 * in a human-readable way.
 */

std::string PolicyCompiler_cisco::mangleInterfaceName(
    const string &interface_name)
{
    string::size_type n;
    string s = interface_name;

    // lowercase all characters
    transform (s.begin(), s.end(),    // source
               s.begin(),             // destination
               ::tolower);              // operation

    map<string,string> name_mapping;
    map<string,string>::iterator nmi;

    name_mapping["async"] = "as";
    name_mapping["atm"] = "atm";
    name_mapping["bri"] = "bri";
    name_mapping["ethernet"] = "e";
    name_mapping["fastethernet"] = "fe";
    name_mapping["fddi"] = "fddi";
    name_mapping["gigabitethernet"] = "ge";
    name_mapping["hssi"] = "h";
    name_mapping["loopback"] = "l";
    name_mapping["port-channel"] = "pc";
    name_mapping["pos"] = "pos";
    name_mapping["serial"] = "s";
    name_mapping["switch"] = "sw";
    name_mapping["tokenring"] = "tr";
    name_mapping["tunnel"] = "tun";
    name_mapping["tengigabitethernet"] = "te";
    name_mapping["sonet"] = "so";
    name_mapping["vg-anylan"] = "vg";

    for (nmi=name_mapping.begin(); nmi!=name_mapping.end(); nmi++)
    {
        if (s.find( nmi->first )==0)
        {
            s.replace(0, nmi->first.size(), nmi->second);
            break;
        }
    }

    while ( (n=s.find(" "))!=string::npos)
    {
        s.replace(n,1,"_");
    }
    while ( (n=s.find("/"))!=string::npos)
    {
        s.replace(n,1,"_");
    }
    return s;
}


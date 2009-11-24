/*

                          Firewall Builder

                 Copyright (C) 2003 NetCitadel, LLC

  Author:  Vadim Kurland     vadim@fwbuilder.org

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



#include "../../config.h"
#include "global.h"
#include "utils.h"

#include "ObjectEditor.h"

#include <qobject.h>
#include <qpixmap.h>
#include <qmessagebox.h>
#include <qmenu.h>
#include <qlayout.h>
#include <qstackedwidget.h>
#include <qpushbutton.h>
#include <QtDebug>
#include <QCoreApplication>

#include "FWWindow.h"
#include "BaseObjectDialog.h"
#include "FirewallDialog.h"
#include "InterfaceDialog.h"
#include "DialogFactory.h"
#include "FWBTree.h"
#include "ProjectPanel.h"
#include "FWBSettings.h"
#include "GroupObjectDialog.h"
#include "ActionsDialog.h"
#include "MetricEditorPanel.h"
#include "CommentEditorPanel.h"
#include "CompilerOutputPanel.h"
#include "ObjectManipulator.h"
#include "Help.h"
#include "events.h"

#include "fwbuilder/Library.h"
#include "fwbuilder/Firewall.h"
#include "fwbuilder/Cluster.h"
#include "fwbuilder/StateSyncClusterGroup.h"
#include "fwbuilder/FailoverClusterGroup.h"
#include "fwbuilder/Host.h"
#include "fwbuilder/Network.h"
#include "fwbuilder/NetworkIPv6.h"
#include "fwbuilder/IPv4.h"
#include "fwbuilder/IPv6.h"
#include "fwbuilder/DNSName.h"
#include "fwbuilder/AddressTable.h"
#include "fwbuilder/AddressRange.h"
#include "fwbuilder/ObjectGroup.h"
#include "fwbuilder/Policy.h"
#include "fwbuilder/NAT.h"
#include "fwbuilder/Routing.h"

#include "fwbuilder/Resources.h"
#include "fwbuilder/FWReference.h"
#include "fwbuilder/Interface.h"
#include "fwbuilder/RuleSet.h"
#include "fwbuilder/Rule.h"

#include "fwbuilder/CustomService.h"
#include "fwbuilder/IPService.h"
#include "fwbuilder/ICMPService.h"
#include "fwbuilder/ICMP6Service.h"
#include "fwbuilder/TCPService.h"
#include "fwbuilder/UDPService.h"
#include "fwbuilder/ServiceGroup.h"
#include "fwbuilder/TagService.h"
#include "fwbuilder/UserService.h"

#include "fwbuilder/Interval.h"
#include "fwbuilder/IntervalGroup.h"

#include <iostream>

using namespace std;
using namespace libfwbuilder;

#define OBJTREEVIEW_WIDGET_NAME  "ObjTreeView"



ObjectEditor::ObjectEditor( QWidget *parent):
    QObject(parent), opened(0), current_dialog_idx(-1), current_dialog_name(""),
    editorStack(dynamic_cast<QStackedWidget*>(parent)),
    helpButton(0),
    m_project(0),
    openedOpt(optNone)
{
    /*
     * To add a dialog for the new object type:
     *
     * - In Designer:
     *   - create new dialog, inherit from QWidget, e.g. class FooDialog
     *   - add new page to objectEditorStack in FWBMainWindow_q
     *   - drop QWidget into this page
     *   - promote this widget to class FooDialog, include file FooDialog.h
     *   - set name of this widget to "w_FooDialog"
     *   - add grid layout to the stack page, set all margins to 0
     * - Add call to registerObjectDialog() here using name "w_FooDialog"
     */
    registerObjectDialog(editorStack, Firewall::TYPENAME, "w_FirewallDialog");
    registerObjectDialog(editorStack, Interface::TYPENAME, "w_InterfaceDialog");
    registerObjectDialog(editorStack, UserService::TYPENAME, "w_UserDialog");
    registerObjectDialog(editorStack, Policy::TYPENAME, "w_PolicyDialog");
    registerObjectDialog(editorStack, NAT::TYPENAME, "w_NATDialog");
    registerObjectDialog(editorStack, Routing::TYPENAME, "w_RoutingDialog");

    registerObjectDialog(editorStack, Library::TYPENAME, "w_LibraryDialog");
    registerObjectDialog(editorStack, IPv4::TYPENAME, "w_IPv4Dialog");
    registerObjectDialog(editorStack, IPv6::TYPENAME, "w_IPv6Dialog");
    registerObjectDialog(editorStack, physAddress::TYPENAME, "w_PhysicalAddressDialog");
    registerObjectDialog(editorStack, AddressRange::TYPENAME, "w_AddressRangeDialog");
    registerObjectDialog(editorStack, Cluster::TYPENAME, "w_ClusterDialog");
    registerObjectDialog(editorStack, FailoverClusterGroup::TYPENAME,
                         "w_FailoverClusterGroupDialog");
    registerObjectDialog(editorStack, StateSyncClusterGroup::TYPENAME,
                         "w_StateSyncClusterGroupDialog");
    registerObjectDialog(editorStack, Host::TYPENAME, "w_HostDialog");
    registerObjectDialog(editorStack, Network::TYPENAME, "w_NetworkDialog");
    registerObjectDialog(editorStack, NetworkIPv6::TYPENAME, "w_NetworkDialogIPv6");
    registerObjectDialog(editorStack, CustomService::TYPENAME, "w_CustomServiceDialog");
    registerObjectDialog(editorStack, IPService::TYPENAME, "w_IPServiceDialog");
    registerObjectDialog(editorStack, ICMPService::TYPENAME, "w_ICMPServiceDialog");
    registerObjectDialog(editorStack, ICMP6Service::TYPENAME, "w_ICMP6ServiceDialog");
    registerObjectDialog(editorStack, TCPService::TYPENAME, "w_TCPServiceDialog");
    registerObjectDialog(editorStack, UDPService::TYPENAME, "w_UDPServiceDialog");
    registerObjectDialog(editorStack, TagService::TYPENAME, "w_TagServiceDialog");
    registerObjectDialog(editorStack, ServiceGroup::TYPENAME, "w_ServiceGroupDialog");
    registerObjectDialog(editorStack, ObjectGroup::TYPENAME, "w_ObjectGroupDialog");
    registerObjectDialog(editorStack, IntervalGroup::TYPENAME, "w_IntervalGroupDialog");
    registerObjectDialog(editorStack, Interval::TYPENAME, "w_TimeDialog");

    registerObjectDialog(editorStack, RoutingRule::TYPENAME, "w_RoutingRuleOptionsDialog");
    registerObjectDialog(editorStack, PolicyRule::TYPENAME, "w_RuleOptionsDialog");
    registerObjectDialog(editorStack, NATRule::TYPENAME, "w_NATRuleOptionsDialog");

    registerObjectDialog(editorStack, AddressTable::TYPENAME, "w_AddressTableDialog");
    registerObjectDialog(editorStack, DNSName::TYPENAME, "w_DNSNameDialog");

    registerOptDialog(editorStack, optAction, "w_ActionsDialog");
    registerOptDialog(editorStack, optComment, "w_CommentEditorPanel");
    registerOptDialog(editorStack, optMetric, "w_MetricEditorPanel");
    registerOptDialog(editorStack, optRuleCompile, "w_CompilerOutputPanel");

    registerObjectDialog(editorStack, "BLANK", "w_BlankDialog");

    // BaseObjectDialog *w = new BaseObjectDialog(parent);
    // stackIds["BLANK"]  = editorStack->addWidget(w);
    // dialogs[stackIds["BLANK"]] = w;
}

void ObjectEditor::registerObjectDialog(QStackedWidget *stack,
                                        const QString &obj_type,
                                        const QString &dialog_name)
{
    BaseObjectDialog *w = qFindChild<BaseObjectDialog*>(stack, dialog_name);
    if (w==NULL)
    {
        qDebug() << "Dialog widget missing for the object type "
                 << obj_type
                 << "  Expected the following name for the dialog object: "
                 << dialog_name;
    }
    assert(w);
    int dlg_id = stack->indexOf(w->parentWidget());
    stackIds[obj_type]  = dlg_id;
    dialogs[dlg_id] = w;
}

void ObjectEditor::registerOptDialog(QStackedWidget *stack,
                                     ObjectEditor::OptType opt_type,
                                     const QString &dialog_name)
{
    BaseObjectDialog *w = qFindChild<BaseObjectDialog*>(stack, dialog_name);
    if (w==NULL)
    {
        qDebug() << "Dialog widget missing for the option "
                 << opt_type
                 << "  Expected the following name for the dialog object: "
                 << dialog_name;
    }
    assert(w);
    int dlg_id = stack->indexOf(w->parentWidget());
    stackIds[getOptDialogName(opt_type)]  = dlg_id;
    dialogs[dlg_id] = w;
}

void ObjectEditor::attachToProjectWindow(ProjectPanel *pp)
{ 
    if (fwbdebug)
        qDebug() << "ObjectEditor::attachToProjectWindow pp=" << pp;

    m_project = pp;

    QMapIterator<int, BaseObjectDialog*> it(dialogs);
    while (it.hasNext())
    {
        it.next();
        it.value()->attachToProjectWindow(pp);
    }
}

QWidget* ObjectEditor::getCurrentObjectDialog()
{
    if (current_dialog_idx >= 0) return dialogs[current_dialog_idx];
    else return NULL;
}

// /*
//  * TODO: deprecate this
//  */
// void ObjectEditor::show()
// {
// }

// /*
//  * TODO: deprecate this
//  *
//  * need to call ProjectPanel::closeEditorPanel from here because
//  * we connect signal clicked() from the closeButton to the slot in
//  * ObjectEditor rather than in ProjectPanel.
//  */
// void ObjectEditor::hide()
// {
//     mw->closeEditorPanel();
//     current_dialog_idx = -1;
//     current_dialog_name = "";
// }

QString ObjectEditor::getOptDialogName(OptType t)
{
    return QString("OptionDialog_%1").arg(t);
}

void ObjectEditor::activateDialog(const QString &dialog_name,
                                  FWObject *obj, enum OptType opt)
{
    disconnectSignals();

    current_dialog_name = dialog_name;
    current_dialog_idx = stackIds[current_dialog_name];
    editorStack->setCurrentIndex(current_dialog_idx);

    connect(this, SIGNAL(loadObject_sign(libfwbuilder::FWObject*)),
            dialogs[ current_dialog_idx ],
            SLOT(loadFWObject(libfwbuilder::FWObject*)));

    opened = obj;
    openedOpt = opt;

    load();
    findAndLoadHelp();

    //show();

    connect(this, SIGNAL(validate_sign(bool*)),
            dialogs[ current_dialog_idx ],
            SLOT(validate(bool*)));

    connect(this, SIGNAL(applyChanges_sign()),
            dialogs[ current_dialog_idx ],
            SLOT(applyChanges()));

    connect(this, SIGNAL(discardChanges_sign()),
            dialogs[ current_dialog_idx ],
            SLOT(discardChanges()));

    connect(dialogs[ current_dialog_idx ], SIGNAL(changed_sign()),
            this,
            SLOT(changed()));

    connect(dialogs[ current_dialog_idx ], SIGNAL(notify_changes_applied_sign()),
            this,
            SLOT(notifyChangesApplied()));

    connect(this, SIGNAL(getHelpName_sign(QString*)),
            dialogs[ current_dialog_idx ],
            SLOT(getHelpName(QString*)));
}

void ObjectEditor::open(FWObject *obj)
{
    if (stackIds.count(obj->getTypeName().c_str())!=0)
    {
        if (fwbdebug) 
            qDebug() << "ObjectEditor::open obj=" << obj
                     << QString((obj)?obj->getName().c_str():"")
                     << QString((obj)?obj->getTypeName().c_str():"");

        activateDialog(obj->getTypeName().c_str(), obj, optNone);
    } else
        blank();
}

void ObjectEditor::openOpt(FWObject *obj, OptType t)
{
    if (stackIds.count(getOptDialogName(t))!=0)
    {
        if (fwbdebug) 
            qDebug() << "ObjectEditor::openOpt obj=" << obj
                     << QString((obj)?obj->getName().c_str():"")
                     << QString((obj)?obj->getTypeName().c_str():"")
                     << "t=" << t;

        if (Rule::cast(obj)==NULL) return;

        activateDialog(getOptDialogName(t), obj, t);
    } else
        blank();
}

void ObjectEditor::disconnectSignals()
{
    disconnect( SIGNAL(loadObject_sign(libfwbuilder::FWObject*)) );
    disconnect( SIGNAL(validate_sign(bool*)) );
    //disconnect( SIGNAL(isChanged_sign(bool*)) );
    disconnect( SIGNAL(applyChanges_sign()) );
    disconnect( SIGNAL(discardChanges_sign()) );
    disconnect( SIGNAL(getHelpName_sign(QString*)) );
    if (current_dialog_idx>=0) dialogs[current_dialog_idx]->disconnect( this );
}

void ObjectEditor::purge()
{
    if (fwbdebug) qDebug("ObjectEditor::purge");
    activateDialog("BLANK", NULL, optNone);
    openedOpt = optNone;
}

void ObjectEditor::setHelpButton(QPushButton * b)
{
    helpButton=b;
    helpButton->setEnabled(true);
    connect((QWidget*)helpButton,SIGNAL(clicked()),this,SLOT(help()));
}

// Object dialog emits signal notify_changes_applied_sign() when it
// applies changes to the object
void ObjectEditor::notifyChangesApplied()
{
    //applyButton->setEnabled(false);
    // send event so other project panels can reload themselves
    // QCoreApplication::postEvent(
    //     mw, new dataModifiedEvent(QString::fromUtf8(opened->getRoot()->getFileName().c_str()),
    //                               opened->getId()));
}

// this method is connected to the "Apply" button
// TODO: deprecate this
void ObjectEditor::apply()
{
    if (fwbdebug) qDebug("ObjectEditor::apply");

    bool isgood = true;
    emit validate_sign( &isgood );

    if (isgood)
        emit applyChanges_sign(); // eventually emits notify_changes_applied_sign()
}

void ObjectEditor::help()
{
    QString help_name;
    emit getHelpName_sign(&help_name);
    if (fwbdebug)
        qDebug("ObjectEditor::help: %s", help_name.toStdString().c_str());
    Help *h = new Help(editorStack, "");
    h->setSource(QUrl(help_name + ".html"));
    h->show();
}

void ObjectEditor::findAndLoadHelp()
{
    QString help_name;
    emit getHelpName_sign(&help_name);

    // is help available?
    Help *h = new Help(editorStack, "");
    helpButton->setEnabled(!h->findHelpFile(help_name + ".html").isEmpty());
    delete h;
}

void ObjectEditor::load()
{
    emit loadObject_sign(opened);
}

void ObjectEditor::changed()
{
    if (fwbdebug) qDebug() << "ObjectEditor::changed()";
    if (!validate())
    {
        // change is not good, reload data into the editor to clear and reset it.
        load();
        return;
    }

    emit applyChanges_sign();
}

bool ObjectEditor::validate()
{
    if (fwbdebug) qDebug() << "ObjectEditor::validate()";
    bool isgood = true;
    emit validate_sign( &isgood );
    return isgood;
}

/*
 * For groups, select object in the group dialog. Argument @o is the
 * child object that has to be selected in the dialog, <opened> is
 * currently opened object (which should be the parent of @o or hold
 * reference to @o)
 */
void ObjectEditor::selectObject(FWObject *o)
{
    qDebug("ObjectEditor::selectObject");
    // class Library inherits Group but has its own dialog where
    // children objects are not shown.
    if (Library::cast(opened)!=NULL || Group::cast(opened)==NULL || current_dialog_idx==-1)
        return;
    ((GroupObjectDialog *) dialogs[current_dialog_idx])->selectObject(o);
}

void ObjectEditor::selectionChanged(FWObject*)
{
}

void ObjectEditor::blank()
{
    if (fwbdebug) qDebug() << "ObjectEditor::blank()";
    purge();
}

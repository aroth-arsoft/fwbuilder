#ifndef INSTDIALOGTEST_H
#define INSTDIALOGTEST_H

#include <QTest>
#include "newClusterDialog.h"
#include "upgradePredicate.h"
#include "FWBTree.h"
#include "fwbuilder/Library.h"
#include "instDialog.h"
#include "FWWindow.h"
#include "ObjectTreeView.h"
#include "ObjectTreeViewItem.h"
#include "events.h"
#include "fwbuilder/Firewall.h"
#include "fwbuilder/Policy.h"

class instDialogTest : public QObject
{
    Q_OBJECT
    void openPolicy(QString fw);
private slots:
    void initTestCase();
    void page1_1();
    void page1_2();
    void page1_3();
    void page1_4();

};

#endif // INSTDIALOGTEST_H

/**
 * MainWindow — 完整专业级实现。
 * 参考 docs/ui/ 参考图。
 */
#include "mainwindow.h"
#include "worker.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QStatusBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QDir>
#include <QMargins>
#include <QFileDialog>
#include <QHeaderView>
#include <QBrush>
#include <QColor>

static const char *THEME = R"(
*{font-family:"Segoe UI","Noto Sans CJK SC",sans-serif;}
QMainWindow,QWidget{background:#1a1a2e;color:#e0e0e0;font-size:13px;}
QWidget#sidebar{background:#16213e;border-right:1px solid #0f3460;}
QLabel#title{font:bold 20px;color:#e94560;}
QLabel#sub{font-size:11px;color:#667788;}
QPushButton#stepBtn{background:transparent;text-align:left;padding:14px 16px;border-radius:8px;font-size:13px;color:#8899aa;border:1px solid transparent;}
QPushButton#stepBtn:hover{background:#1a1a3e;border-color:#0f3460;}
QPushButton#stepBtn[active="true"]{background:#0f3460;color:#e94560;font-weight:bold;border:1px solid #e94560;}
QPushButton#importBtn{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #e94560,stop:1 #c73e54);color:white;border:none;border-radius:8px;padding:14px;font:bold 14px;}
QPushButton#importBtn:hover{background:#c73e54;}
QPushButton#importBtn:disabled{background:#333;color:#666;}
QLineEdit{background:#0f3460;border:1px solid #1a4a8a;border-radius:4px;padding:7px 10px;color:#e0e0e0;font-size:12px;}
QLineEdit:focus{border-color:#e94560;}
QLineEdit:read-only{background:#16213e;color:#8899aa;}
QGroupBox{border:1px solid #1a2a4a;border-radius:6px;margin-top:14px;padding-top:18px;font-weight:bold;color:#53a8b6;font-size:12px;}
QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 6px;}
QRadioButton{color:#c0c0c0;spacing:8px;font-size:12px;}
QRadioButton::indicator{width:16px;height:16px;border:2px solid #1a4a8a;border-radius:8px;}
QRadioButton::indicator:checked{background:#e94560;border-color:#e94560;}
QCheckBox{color:#c0c0c0;spacing:8px;}
QTableWidget{background:#0f1225;alternate-background:#131730;color:#e0e0e0;gridline-color:#1a2a4a;border:1px solid #1a2a4a;border-radius:6px;font-size:12px;selection-background-color:#0f3460;}
QTableWidget::item{padding:5px 8px;}
QHeaderView::section{background:#16213e;color:#53a8b6;border:none;border-bottom:2px solid #0f3460;padding:8px;font-weight:bold;}
QComboBox{background:#0f3460;border:1px solid #1a4a8a;border-radius:4px;padding:5px 10px;color:#e0e0e0;}
QComboBox QAbstractItemView{background:#16213e;color:#e0e0e0;selection-background:#0f3460;}
QTextEdit#log{background:#0a0e1a;border:1px solid #1a2a4a;color:#8899aa;font:11px "Cascadia Code",Consolas,monospace;border-radius:4px;}
QTextEdit#pdo{background:#0a0e1a;border:1px solid #1a2a4a;color:#53a8b6;font:11px "Cascadia Code",Consolas,monospace;border-radius:4px;}
QProgressBar{border:1px solid #1a2a4a;border-radius:6px;text-align:center;color:white;font-size:11px;background:#0f1225;max-height:14px;}
QProgressBar::chunk{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #e94560,stop:1 #53a8b6);border-radius:5px;}
QPushButton#genBtn{background:#0f3460;color:#e0e0e0;border:1px solid #1a4a8a;border-radius:6px;padding:10px 18px;font-size:12px;}
QPushButton#genBtn:hover{background:#1a4a8a;border-color:#e94560;}
QStatusBar{background:#16213e;color:#8899aa;border-top:1px solid #1a2a4a;}
)";

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_worker(nullptr) {
    setWindowTitle("NekoECAT Converter");
    setMinimumSize(1200,750); resize(1400,850);
    setStyleSheet(THEME);

    auto *root=new QWidget;
    auto *hl=new QHBoxLayout(root);
    hl->setContentsMargins(QMargins(0,0,0,0)); hl->setSpacing(0);
    hl->addWidget(buildSidebar(),0);
    hl->addWidget(buildRightPanel(),1);
    setCentralWidget(root);
    statusBar()->showMessage("就绪");
    connectSignals(); onStepChanged(0);
}

// ═══ 侧边栏 ═══════════════════════════════════════════════════
QWidget *MainWindow::buildSidebar() {
    auto *w=new QWidget; w->setObjectName("sidebar"); w->setFixedWidth(300);
    auto *L=new QVBoxLayout(w);
    L->setContentsMargins(QMargins(16,20,16,16)); L->setSpacing(4);

    auto *t=new QLabel("NekoECAT Converter"); t->setObjectName("title");
    auto *s=new QLabel("ECAT -> SSC Tool");    s->setObjectName("sub");
    L->addWidget(t); L->addWidget(s); L->addSpacing(12);

    const char *sl[]={"📁 文件导入","🏷 身份配置","⚙ 规则策略"};
    for(int i=0;i<3;i++){
        m_stepBtns[i]=new QPushButton(sl[i]);
        m_stepBtns[i]->setObjectName("stepBtn");
        m_stepBtns[i]->setCursor(Qt::PointingHandCursor);
        int idx=i;
        connect(m_stepBtns[i],&QPushButton::clicked,this,[this,idx]{onStepChanged(idx);});
        L->addWidget(m_stepBtns[i]);
    }
    L->addSpacing(8);

    m_steps=new QStackedWidget;

    // Step 1: 文件
    {
        auto *p=new QWidget; auto *g=new QVBoxLayout(p); g->setContentsMargins(0,0,0,0); g->setSpacing(6);
        auto mkrow=[&](const char*lb,QLineEdit*&ed,const char*flt){
            auto *r=new QWidget; auto *h=new QHBoxLayout(r); h->setContentsMargins(0,0,0,0); h->setSpacing(4);
            auto *l=new QLabel(lb); l->setFixedWidth(40); l->setStyleSheet("color:#667788;font-size:11px;");
            ed=new QLineEdit; ed->setPlaceholderText("...");
            auto *b=new QPushButton("..."); b->setFixedWidth(30);
            connect(b,&QPushButton::clicked,this,[this,ed,flt]{auto p=QFileDialog::getOpenFileName(this,"","",flt);if(!p.isEmpty())ed->setText(p);});
            h->addWidget(l); h->addWidget(ed,1); h->addWidget(b); g->addWidget(r);
        };
        mkrow("ESI:",m_esiEdit,"XML (*.xml);;All (*)");
        mkrow("SDO:",m_sdoEdit,"XML/TXT (*.xml *.txt);;All (*)");
        mkrow("模板:",m_tplEdit,"Excel (*.xlsx);;All (*)");
        auto *r=new QWidget; auto *h=new QHBoxLayout(r); h->setContentsMargins(0,0,0,0); h->setSpacing(4);
        auto *l=new QLabel("输出:"); l->setFixedWidth(40); l->setStyleSheet("color:#667788;font-size:11px;");
        m_outEdit=new QLineEdit("out");
        auto *b=new QPushButton("..."); b->setFixedWidth(30);
        connect(b,&QPushButton::clicked,this,[this]{auto p=QFileDialog::getExistingDirectory(this,"");if(!p.isEmpty())m_outEdit->setText(p);});
        h->addWidget(l); h->addWidget(m_outEdit,1); h->addWidget(b); g->addWidget(r);
        m_steps->addWidget(p);
    }

    // Step 2: 身份
    {
        auto *p=new QWidget; auto *g=new QVBoxLayout(p); g->setContentsMargins(0,0,0,0); g->setSpacing(6);
        auto *og=new QGroupBox("原始身份"); auto *oG=new QGridLayout(og); oG->setSpacing(3);
        const char *ok[]={"厂商:","产品:","修订:","名称:"};
        QLabel *ol[]={nullptr,nullptr,nullptr,nullptr};
        for(int i=0;i<4;i++){
            auto *l=new QLabel(ok[i]); l->setStyleSheet("color:#667788;font-size:11px;");
            ol[i]=new QLabel("-"); ol[i]->setStyleSheet("color:#8899aa;font-size:11px;");
            oG->addWidget(l,i,0); oG->addWidget(ol[i],i,1);
        }
        m_ov=ol[0]; m_op=ol[1]; m_or=ol[2]; m_on=ol[3];
        g->addWidget(og);
        auto *ng=new QGroupBox("新身份"); auto *nG=new QGridLayout(ng); nG->setSpacing(3);
        auto f=[&](int r,const char*lb,QLineEdit*&ed,const char*ph=""){
            auto *l=new QLabel(lb); l->setStyleSheet("color:#667788;font-size:11px;");
            ed=new QLineEdit; ed->setPlaceholderText(ph);
            nG->addWidget(l,r,0); nG->addWidget(ed,r,1);
        };
        f(0,"厂商ID:",m_nv,"0x..."); f(1,"产品码:",m_np,"0x...");
        f(2,"修订:",m_nr,"0x..."); f(3,"名称:",m_nn,"设备名"); f(4,"Physics:",m_phys,"YY");
        g->addWidget(ng);
        auto *mg=new QGroupBox("模式"); auto *mH=new QHBoxLayout(mg);
        m_modeGrp=new QButtonGroup(this);
        const char *mn[]={"学习","实验室","产品"};
        for(int i=0;i<3;i++){auto *r=new QRadioButton(mn[i]);m_modeGrp->addButton(r,i);mH->addWidget(r);}
        m_modeGrp->button(0)->setChecked(true);
        g->addWidget(mg); g->addStretch();
        m_steps->addWidget(p);
    }

    // Step 3: 规则
    {
        auto *p=new QWidget; auto *g=new QVBoxLayout(p); g->setContentsMargins(0,0,0,0); g->setSpacing(6);
        auto mkgrp=[&](const char*title,QButtonGroup*&grp,const char**opts,int n){
            auto *gr=new QGroupBox(title); auto *v=new QVBoxLayout(gr); v->setSpacing(2);
            grp=new QButtonGroup(this);
            for(int i=0;i<n;i++){auto *r=new QRadioButton(opts[i]);grp->addButton(r,i);v->addWidget(r);}
            grp->button(0)->setChecked(true); g->addWidget(gr);
        };
        const char *o1[]={"STRING+callback","跳过","手动"}; mkgrp("OCTET_STRING",m_octGrp,o1,3);
        const char *o2[]={"CoE callback","padding","跳过"}; mkgrp("未对齐对象",m_alnGrp,o2,3);
        const char *o3[]={"SSC自动","强制导入","跳过"};     mkgrp("通信对象",m_comGrp,o3,3);
        auto *fg=new QGroupBox("Fxxx对象"); auto *fH=new QHBoxLayout(fg);
        m_fxxGrp=new QButtonGroup(this);
        const char *o4[]={"包含","跳过","callback"};
        for(int i=0;i<3;i++){auto *r=new QRadioButton(o4[i]);m_fxxGrp->addButton(r,i);fH->addWidget(r);}
        m_fxxGrp->button(0)->setChecked(true); g->addWidget(fg); g->addStretch();
        m_steps->addWidget(p);
    }

    L->addWidget(m_steps,1);

    m_importBtn=new QPushButton("📥  导入并解析");
    m_importBtn->setObjectName("importBtn");
    m_importBtn->setMinimumHeight(46);
    m_importBtn->setEnabled(false);
    connect(m_importBtn,&QPushButton::clicked,this,&MainWindow::onImport);
    L->addWidget(m_importBtn);
    m_fileStatus=new QLabel("未选择文件");
    m_fileStatus->setStyleSheet("color:#667788;font-size:11px;");
    L->addWidget(m_fileStatus);
    L->addStretch();
    auto *v=new QLabel("v0.1.0 | C++ Qt5"); v->setStyleSheet("color:#334455;font-size:10px;");
    L->addWidget(v);
    return w;
}

// ═══ 右侧 ═══════════════════════════════════════════════════
QWidget *MainWindow::buildRightPanel() {
    auto *w=new QWidget;
    auto *vl=new QVBoxLayout(w);
    vl->setContentsMargins(QMargins(12,12,12,0)); vl->setSpacing(8);

    // 对象表格
    auto *tg=new QGroupBox("对象字典");
    auto *tL=new QVBoxLayout(tg);
    auto *fr=new QHBoxLayout;
    auto *fl=new QLabel("过滤:"); fl->setStyleSheet("color:#8899aa;");
    m_filter=new QComboBox;
    m_filter->addItems({"全部","仅错误","仅callback","仅PDO","仅Fxxx"});
    m_objCnt=new QLabel("0 对象"); m_objCnt->setStyleSheet("color:#53a8b6;");
    fr->addWidget(fl); fr->addWidget(m_filter); fr->addStretch(); fr->addWidget(m_objCnt);
    tL->addLayout(fr);
    m_table=new QTableWidget;
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels({"Index","Name","Sub","Cat","PDO","SM","Risk","CoeR","CoeW"});
    m_table->setEditTriggers(QTableWidget::NoEditTriggers);
    m_table->setSelectionBehavior(QTableWidget::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    tL->addWidget(m_table);
    vl->addWidget(tg,2);

    // PDO
    auto *pg=new QGroupBox("PDO / SM 校验");
    auto *pL=new QVBoxLayout(pg);
    m_pdoTxt=new QTextEdit; m_pdoTxt->setObjectName("pdo");
    m_pdoTxt->setReadOnly(true); m_pdoTxt->setPlaceholderText("解析后显示...");
    pL->addWidget(m_pdoTxt);
    vl->addWidget(pg,1);

    vl->addWidget(buildBottomBar(),0);
    return w;
}

QWidget *MainWindow::buildBottomBar() {
    auto *bar=new QWidget;
    bar->setStyleSheet("background:#16213e;border-top:1px solid #1a2a4a;");
    auto *bl=new QVBoxLayout(bar);
    bl->setContentsMargins(QMargins(12,8,12,8)); bl->setSpacing(6);

    auto *br=new QHBoxLayout;
    struct B{const char*l;int m;};
    B bs[]={{"🚀 全部生成",0},{"📊 SSC xlsx",1},{"📋 报告",2},{"📄 ESI XML",3}};
    for(int i=0;i<4;i++){
        auto *btn=new QPushButton(bs[i].l); btn->setObjectName("genBtn"); btn->setMinimumWidth(130);
        int m=bs[i].m; connect(btn,&QPushButton::clicked,this,[this,m]{onGenerate(m);});
        br->addWidget(btn);
    }
    br->addStretch(); bl->addLayout(br);

    m_prog=new QProgressBar; m_prog->setRange(0,100); m_prog->setValue(0);
    bl->addWidget(m_prog);
    m_log=new QTextEdit; m_log->setObjectName("log"); m_log->setReadOnly(true);
    m_log->setMaximumHeight(80); m_log->setPlaceholderText("操作日志...");
    bl->addWidget(m_log);
    return bar;
}

// ═══ 信号 ═══════════════════════════════════════════════════
void MainWindow::connectSignals() {
    auto ck=[this]{
        bool e=!m_esiEdit->text().trimmed().isEmpty();
        bool s=!m_sdoEdit->text().trimmed().isEmpty();
        m_importBtn->setEnabled(e||s);
        m_fileStatus->setText(e||s?QString("已选: %1%2").arg(e?"ESI ":"").arg(s?"SDO":""):"未选择文件");
    };
    connect(m_esiEdit,&QLineEdit::textChanged,this,ck);
    connect(m_sdoEdit,&QLineEdit::textChanged,this,ck);
}

void MainWindow::onStepChanged(int s) {
    m_steps->setCurrentIndex(s);
    for(int i=0;i<3;i++){
        m_stepBtns[i]->setProperty("active",i==s);
        m_stepBtns[i]->style()->unpolish(m_stepBtns[i]);
        m_stepBtns[i]->style()->polish(m_stepBtns[i]);
    }
}

// ═══ 导入 ═══════════════════════════════════════════════════
void MainWindow::onImport() {
    QString esi=m_esiEdit->text().trimmed(),sdo=m_sdoEdit->text().trimmed();
    if(esi.isEmpty()&&sdo.isEmpty()){setStatus("选择文件");return;}
    setStatus("解析中..."); m_prog->setValue(10); log("[导入] 开始...");
    QString root=QDir::currentPath();
    QString py="import sys,json;sys.path.insert(0,'.');from nekoecat.core import parse_only;"
               "e=sys.argv[1] if sys.argv[1]!='None' else None;"
               "s=sys.argv[2] if sys.argv[2]!='None' else None;"
               "d=parse_only(esi_path=e,sdo_path=s);"
               "print(json.dumps(d.model_dump(mode='json'),ensure_ascii=False))";
    QProcess proc; proc.setWorkingDirectory(root);
    proc.start("python3",{"-c",py,esi.isEmpty()?"None":esi,sdo.isEmpty()?"None":sdo});
    proc.waitForFinished(30000);
    if(proc.exitCode()!=0){log("[错误] "+QString::fromUtf8(proc.readAllStandardError()));setStatus("失败");m_prog->setValue(0);return;}
    m_devJson=QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    m_prog->setValue(50);
    QJsonObject dev=QJsonDocument::fromJson(m_devJson.toUtf8()).object();
    QJsonObject id=dev["identity"].toObject();
    m_ov->setText(QString("0x%1").arg((quint64)id["vendor_id"].toDouble(),8,16,QChar('0')));
    m_op->setText(QString("0x%1").arg((quint64)id["product_code"].toDouble(),8,16,QChar('0')));
    m_or->setText(QString("0x%1").arg((quint64)id["revision_number"].toDouble(),8,16,QChar('0')));
    m_on->setText(id["device_name"].toString());
    updateObjectTable(dev["objects"].toObject());
    updatePdoPanel(dev);
    int n=dev["objects"].toObject().keys().size();
    m_prog->setValue(60); log(QString("[完成] %1 对象").arg(n));
    setStatus(QString("解析完成 — %1 对象").arg(n));
}

// ═══ 对象表格 ═══════════════════════════════════════════════
void MainWindow::updateObjectTable(const QJsonObject &objs) {
    m_table->setRowCount(0);
    QStringList keys=objs.keys();
    std::sort(keys.begin(),keys.end(),[](const QString&a,const QString&b){return a.toInt()<b.toInt();});
    m_table->setRowCount(keys.size());
    for(int r=0;r<keys.size();r++){
        QJsonObject o=objs[keys[r]].toObject();
        int idx=keys[r].toInt();
        QColor bg(15,18,37);
        bool risk=!o["risk_flags"].toArray().isEmpty();
        bool coe=o["coe_read"].toBool()||o["coe_write"].toBool();
        bool pdo=o["is_pdo_mapping_object"].toBool();
        if(risk)bg=QColor(60,20,20); else if(coe)bg=QColor(40,35,15); else if(pdo)bg=QColor(15,30,15);
        auto at=[&](int c,const QString&t){
            auto *i=new QTableWidgetItem(t); i->setBackground(QBrush(bg)); i->setForeground(QBrush(QColor(200,200,200)));
            m_table->setItem(r,c,i);
        };
        at(0,QString("0x%1").arg(idx,4,16,QChar('0')).toUpper());
        at(1,o["name"].toString());
        at(2,QString::number(o["subindices"].toArray().size()));
        at(3,o["category"].toString());
        at(4,pdo?"✓":"");
        at(5,o["is_sm_object"].toBool()?"✓":"");
        at(6,risk?o["risk_flags"].toArray()[0].toString():"");
        at(7,o["coe_read"].toBool()?"TRUE":"");
        at(8,o["coe_write"].toBool()?"TRUE":"");
    }
    m_objCnt->setText(QString("%1 对象").arg(keys.size()));
}

void MainWindow::updatePdoPanel(const QJsonObject &dev) {
    QStringList L;
    for(const auto&sv:dev["sm_configs"].toArray()){
        QJsonObject sm=sv.toObject();
        int def=sm["default_size"].toInt(),calc=0;
        for(const auto&pv:sm["pdos"].toArray())
            for(const auto&ev:pv.toObject()["entries"].toArray())
                calc+=ev.toObject()["bit_length"].toInt();
        calc=(calc+7)/8;
        L<<QString("SM%1 [%2]: 默认=%3B 计算=%4B %5")
            .arg(sm["index"].toInt()).arg(sm["pdo_type"].toString())
            .arg(def).arg(calc).arg((def==calc||calc==0)?"✅ OK":"❌ MISMATCH");
    }
    m_pdoTxt->setPlainText(L.isEmpty()?"(无配置)":L.join("\n"));
}

void MainWindow::onGenerate(int) {
    if(m_devJson.isEmpty()){setStatus("先导入");return;}
    setStatus("转换中..."); m_prog->setValue(10); log("[生成] 启动...");
    QVariantMap id; id["vendor_id"]=m_nv->text().trimmed(); id["product_code"]=m_np->text().trimmed();
    id["revision"]=m_nr->text().trimmed(); id["device_name"]=m_nn->text().trimmed();
    m_worker=new ConvertWorker(m_esiEdit->text().trimmed(),m_sdoEdit->text().trimmed(),
        m_tplEdit->text().trimmed(),m_outEdit->text().trimmed(),id,QVariantMap(),this);
    connect(m_worker,&ConvertWorker::log,this,[this](const QString&m){log(m);});
    connect(m_worker,&ConvertWorker::progress,m_prog,&QProgressBar::setValue);
    connect(m_worker,&ConvertWorker::finishedOk,this,[this](const QString&d){log("[完成] "+d);setStatus("完成");m_prog->setValue(100);});
    connect(m_worker,&ConvertWorker::failed,this,[this](const QString&e){log("[失败] "+e);setStatus("失败");m_prog->setValue(0);});
    connect(m_worker,&ConvertWorker::finished,m_worker,&QObject::deleteLater);
    m_worker->start();
}

void MainWindow::setStatus(const QString&m){statusBar()->showMessage(m);}
void MainWindow::log(const QString&m){m_log->append(m);}

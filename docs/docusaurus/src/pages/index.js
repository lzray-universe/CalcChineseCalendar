import Link from '@docusaurus/Link';
import Layout from '@theme/Layout';
import styles from './index.module.css';

const quickLinks = [
  {
    title: '项目概览',
    description: '了解项目定位、核心能力和文档结构。',
    href: '/docs/intro',
    action: '查看总览',
  },
  {
    title: '快速开始',
    description: '先构建，再跑几个最常用的命令。',
    href: '/docs/quick-start',
    action: '快速开始',
  },
  {
    title: 'CLI 概览',
    description: '按命令族浏览日历、搜索、天文和工具命令。',
    href: '/docs/cli-overview',
    action: '查看命令',
  },
  {
    title: '语言绑定',
    description: '查看 C++、C、Python 和 npm 的接入方式。',
    href: '/docs/bindings',
    action: '查看接入',
  },
  {
    title: '架构与仓库结构',
    description: '从源码目录理解项目的主要分层。',
    href: '/docs/architecture',
    action: '查看结构',
  },
];

function SectionCard({title, description, href, action}) {
  return (
    <article className={styles.card}>
      <h3>{title}</h3>
      <p>{description}</p>
      <Link className={styles.cardLink} to={href}>
        {action}
      </Link>
    </article>
  );
}

export default function Home() {
  return (
    <Layout
      title="CalcChineseCalendar Docs"
      description="CalcChineseCalendar 的 Docusaurus 文档站点"
    >
      <main>
        <section className={styles.hero}>
          <div className="container">
            <div className={styles.heroCopy}>
              <div className={styles.kicker}>CALC CHINESE CALENDAR</div>
              <h1>CalcChineseCalendar 文档</h1>
              <p>
                覆盖项目概览、构建方式、BSP 运行规则、CLI 命令和多语言接入方式。
              </p>
              <div className={styles.heroActions}>
                <Link className="button button--primary button--lg" to="/docs/quick-start">
                  快速开始
                </Link>
                <Link className="button button--secondary button--lg" to="/docs/intro">
                  查看文档
                </Link>
              </div>
              <div className={styles.heroMeta}>
                <span>CLI</span>
                <span>C++ / C API</span>
                <span>Python</span>
                <span>npm / wasm</span>
              </div>
            </div>
          </div>
        </section>

        <section className={styles.section}>
          <div className="container">
            <div className={styles.sectionHeader}>
              <span className={styles.sectionLabel}>Quick Links</span>
              <h2>常用入口</h2>
            </div>
            <div className={styles.cardGrid}>
              {quickLinks.map((section) => (
                <SectionCard key={section.href} {...section} />
              ))}
            </div>
            <p className={styles.sectionNote}>
              参数级细节目前仍可继续参考仓库根目录的 `README.md` 与 `README_zh.md`。
            </p>
          </div>
        </section>
      </main>
    </Layout>
  );
}

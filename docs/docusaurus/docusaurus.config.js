import {themes as prismThemes} from 'prism-react-renderer';

const config = {
  title: 'CalcChineseCalendar Docs',
  tagline: '面向历法、黄历与天文计算的文档站点',
  favicon: 'img/logo.svg',

  url: 'https://lzray-universe.github.io',
  baseUrl: '/CalcChineseCalendar/',

  organizationName: 'lzray-universe',
  projectName: 'CalcChineseCalendar',
  trailingSlash: true,

  onBrokenLinks: 'throw',
  onBrokenAnchors: 'warn',

  i18n: {
    defaultLocale: 'zh-Hans',
    locales: ['zh-Hans', 'en'],
  },

  presets: [
    [
      'classic',
      {
        docs: {
          sidebarPath: './sidebars.js',
          routeBasePath: 'docs',
        },
        blog: false,
        theme: {
          customCss: './src/css/custom.css',
        },
      },
    ],
  ],

  themeConfig: {
    image: 'img/social-card.svg',
    navbar: {
      title: 'CalcChineseCalendar',
      logo: {
        alt: 'CalcChineseCalendar Logo',
        src: 'img/logo.svg',
      },
      items: [
        {
          type: 'docSidebar',
          sidebarId: 'docsSidebar',
          position: 'left',
          label: '文档',
        },
        {
          to: '/docs/quick-start',
          label: '快速开始',
          position: 'left',
        },
        {
          type: 'localeDropdown',
          position: 'right',
        },
        {
          href: 'https://github.com/lzray-universe/CalcChineseCalendar',
          label: 'GitHub',
          position: 'right',
        },
      ],
    },
    footer: {
      style: 'dark',
      links: [
        {
          title: '文档',
          items: [
            {
              label: '项目概览',
              to: '/docs/intro',
            },
            {
              label: '快速开始',
              to: '/docs/quick-start',
            },
            {
              label: '命令行使用方法',
              to: '/docs/cli-overview',
            },
          ],
        },
        {
          title: '集成',
          items: [
            {
              label: '语言绑定',
              to: '/docs/bindings',
            },
            {
              label: '架构与仓库结构',
              to: '/docs/architecture',
            },
          ],
        },
        {
          title: '链接',
          items: [
            {
              label: 'README',
              href: 'https://github.com/lzray-universe/CalcChineseCalendar/blob/main/README.md',
            },
            {
              label: 'README_zh',
              href: 'https://github.com/lzray-universe/CalcChineseCalendar/blob/main/README_zh.md',
            },
          ],
        },
      ],
      copyright: `Copyright ${new Date().getFullYear()} CalcChineseCalendar.`,
    },
    prism: {
      theme: prismThemes.github,
      darkTheme: prismThemes.dracula,
      additionalLanguages: ['bash', 'powershell', 'cpp', 'python', 'json'],
    },
    colorMode: {
      defaultMode: 'light',
      respectPrefersColorScheme: true,
    },
  },
};

export default config;

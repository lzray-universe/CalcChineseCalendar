const sidebars = {
  docsSidebar: [
    'intro',
    {
      type: 'category',
      label: '开始使用',
      items: ['quick-start', 'runtime-and-ephemeris'],
    },
    {
      type: 'category',
      label: '命令行使用方法',
      items: [
        'cli-overview',
        {
          type: 'category',
          label: '子命令参考',
          items: [
            'cli/months',
            'cli/calendar',
            'cli/year',
            'cli/event',
            'cli/download',
            'cli/at',
            'cli/convert',
            'cli/day',
            'cli/monthview',
            'cli/export',
            'cli/next',
            'cli/range',
            'cli/search',
            'cli/eclipse',
            'cli/festival',
            'cli/almanac',
            'cli/info',
            'cli/config',
            'cli/completion',
            'cli/zodiac',
            'cli/sky',
          ],
        },
      ],
    },
    {
      type: 'category',
      label: '集成与实现',
      items: ['bindings', 'architecture'],
    },
  ],
};

export default sidebars;

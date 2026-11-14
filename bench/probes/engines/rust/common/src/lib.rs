use criterion::Criterion;
use std::hint::black_box;

pub struct Simple {
    pub title: String,
    pub body: String,
}

pub struct Stock {
    pub symbol: String,
    pub url: String,
    pub name: String,
    pub price: String,
    pub change: String,
    pub ratio: String,
}

pub struct Item {
    pub row_class: String,
    pub index: String,
    pub negative_class: String,
    pub value: Stock,
}

pub struct Tag {
    pub name: String,
}

pub struct Article {
    pub title: String,
    pub url: String,
    pub author: String,
    pub date: String,
    pub teaser: String,
    pub tags: Vec<Tag>,
}

pub struct News {
    pub title: String,
    pub articles: Vec<Article>,
}

pub const STYLE: &str = "\n\t\t/*<![CDATA[*/\n\t\tbody {\n\t\t\tcolor: #333333;\n\t\t\tline-height: 150%;\n\t\t}\n\n\t\tthead {\n\t\t\tfont-weight: bold;\n\t\t\tbackground-color: #CCCCCC;\n\t\t}\n\n\t\t.odd {\n\t\t\tbackground-color: #FFCCCC;\n\t\t}\n\n\t\t.even {\n\t\t\tbackground-color: #CCCCFF;\n\t\t}\n\n\t\t.minus {\n\t\t\tcolor: #FF0000;\n\t\t}\n\n\t\t/*]]>*/\n\t";

pub fn simple() -> Simple {
    Simple {
        title: "Hello, Ramhorns!".to_string(),
        body: "This is a really simple test of the rendering!".to_string(),
    }
}

pub fn stock_items() -> Vec<Item> {
    const STOCKS: [[&str; 6]; 20] = [
        ["ADBE", "http://www.adobe.com", "Adobe Systems", "39.26", "0.13", "0.33"],
        ["AMD", "http://www.amd.com", "Advanced Micro Devices", "16.22", "0.17", "1.06"],
        ["AMZN", "http://www.amazon.com", "Amazon.com", "36.85", "-0.23", "-0.62"],
        ["AAPL", "http://www.apple.com", "Apple", "85.38", "-0.87", "-1.01"],
        ["BEAS", "http://www.bea.com", "BEA Systems", "12.46", "0.09", "0.73"],
        ["CA", "http://www.ca.com", "CA", "24.66", "0.38", "1.57"],
        ["CSCO", "http://www.cisco.com", "Cisco Systems", "26.35", "0.13", "0.5"],
        ["DELL", "http://www.dell.com/", "Dell", "23.73", "-0.42", "-1.74"],
        ["EBAY", "http://www.ebay.com", "eBay", "31.65", "-0.8", "-2.47"],
        ["GOOG", "http://www.google.com", "Google", "495.84", "7.75", "1.59"],
        ["HPQ", "http://www.hp.com", "Hewlett-Packard", "41.69", "-0.02", "-0.05"],
        ["IBM", "http://www.ibm.com", "IBM", "97.45", "-0.06", "-0.06"],
        ["INTC", "http://www.intel.com", "Intel", "20.53", "-0.07", "-0.34"],
        ["JNPR", "http://www.juniper.net/", "Juniper Networks", "18.96", "0.5", "2.71"],
        ["MSFT", "http://www.microsoft.com", "Microsoft", "30.6", "0.15", "0.49"],
        ["ORCL", "http://www.oracle.com", "Oracle", "17.15", "0.17", "1.1"],
        ["SAP", "http://www.sap.com", "SAP", "46.2", "-0.16", "-0.35"],
        ["STX", "http://www.seagate.com/", "Seagate Technology", "27.35", "-0.36", "-1.3"],
        ["SUNW", "http://www.sun.com", "Sun Microsystems", "6.33", "-0.01", "-0.16"],
        ["YHOO", "http://www.yahoo.com", "Yahoo", "28.04", "-0.17", "-0.6"],
    ];
    STOCKS
        .iter()
        .enumerate()
        .map(|(i, s)| {
            let up = !s[4].starts_with('-') && s[4] != "0";
            Item {
                row_class: if (i + 1) % 2 == 0 { "even" } else { "odd" }.to_string(),
                index: (i + 1).to_string(),
                negative_class: if up { "" } else { "class=\"minus\"" }.to_string(),
                value: Stock {
                    symbol: s[0].to_string(),
                    url: s[1].to_string(),
                    name: s[2].to_string(),
                    price: s[3].to_string(),
                    change: s[4].to_string(),
                    ratio: s[5].to_string(),
                },
            }
        })
        .collect()
}

pub fn news() -> News {
    News {
        title: "Today's news".to_string(),
        articles: (0..20)
            .map(|i| {
                let n = (i + 1).to_string();
                Article {
                    title: format!("Article {n}: what happened & why"),
                    url: format!("https://news.example/articles/{n}"),
                    author: format!("Author {n}"),
                    date: format!("2026-09-{}{n}", if i < 9 { "0" } else { "" }),
                    teaser: format!("A teaser of about one line for article {n}, with <b>markup</b> to escape."),
                    tags: vec![
                        Tag { name: "world".to_string() },
                        Tag { name: "tech".to_string() },
                        Tag { name: format!("tag-{n}") },
                    ],
                }
            })
            .collect(),
    }
}

pub fn run(engine: &str, renders: [&dyn Fn() -> String; 3]) {
    let args: Vec<String> = std::env::args().collect();
    if let Some(position) = args.iter().position(|a| a == "--output-dir") {
        let directory = &args[position + 1];
        for (i, render) in renders.iter().enumerate() {
            std::fs::write(format!("{directory}/w{}.html", i + 1), render()).expect("write output");
        }
        return;
    }
    let mut criterion = Criterion::default().configure_from_args();
    let mut group = criterion.benchmark_group(engine);
    for (i, render) in renders.iter().enumerate() {
        group.bench_function(format!("w{}", i + 1), |b| b.iter(|| black_box(render())));
    }
    group.finish();
    criterion.final_summary();
}

use sailfish::TemplateSimple;
use sailfish_bench::{News, Simple, Stocks};

fn main() {
    let simple = common::simple();
    let stock_items = common::stock_items();
    let news = common::news();
    common::run(
        "sailfish",
        [
            &|| Simple { title: &simple.title, body: &simple.body }.render_once().unwrap(),
            &|| Stocks { items: &stock_items }.render_once().unwrap(),
            &|| News { title: &news.title, articles: &news.articles }.render_once().unwrap(),
        ],
    );
}

use markup_bench::{News, Simple, Stocks};

fn main() {
    let simple = common::simple();
    let stock_items = common::stock_items();
    let news = common::news();
    common::run(
        "markup",
        [
            &|| Simple { title: &simple.title, body: &simple.body }.to_string(),
            &|| Stocks { items: &stock_items }.to_string(),
            &|| News { title: &news.title, articles: &news.articles }.to_string(),
        ],
    );
}

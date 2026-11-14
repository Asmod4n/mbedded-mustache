fn main() {
    let simple = common::simple();
    let stock_items = common::stock_items();
    let news = common::news();
    common::run(
        "maud",
        [
            &|| maud_bench::simple(&simple.title, &simple.body),
            &|| maud_bench::stocks(&stock_items),
            &|| maud_bench::news(&news.title, &news.articles),
        ],
    );
}

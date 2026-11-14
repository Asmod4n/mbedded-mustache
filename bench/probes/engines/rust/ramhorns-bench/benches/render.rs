use ramhorns::Template;

fn main() {
    let simple = common::simple();
    let stock_items = common::stock_items();
    let news = common::news();
    let hello = ramhorns_bench::hello(&simple);
    let stocks = ramhorns_bench::stocks(&stock_items);
    let page = ramhorns_bench::page(&news);
    let t1 = Template::new(include_str!("../templates/w1.html")).expect("template 1");
    let t2 = Template::new(include_str!("../templates/w2.html")).expect("template 2");
    let t3 = Template::new(include_str!("../templates/w3.html")).expect("template 3");
    common::run(
        "ramhorns",
        [&|| t1.render(&hello), &|| t2.render(&stocks), &|| t3.render(&page)],
    );
}

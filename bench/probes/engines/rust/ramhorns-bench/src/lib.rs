use common::{Item, News, Simple};
use ramhorns::Content;

#[derive(Content)]
pub struct Stock<'a> {
    pub symbol: &'a str,
    pub url: &'a str,
    pub name: &'a str,
    pub price: &'a str,
    pub change: &'a str,
    pub ratio: &'a str,
}

#[derive(Content)]
pub struct Row<'a> {
    pub row_class: &'a str,
    pub index: &'a str,
    pub negative_class: &'a str,
    pub value: Stock<'a>,
}

#[derive(Content)]
pub struct Stocks<'a> {
    pub items: Vec<Row<'a>>,
}

#[derive(Content)]
pub struct Tag<'a> {
    pub name: &'a str,
}

#[derive(Content)]
pub struct Article<'a> {
    pub title: &'a str,
    pub url: &'a str,
    pub author: &'a str,
    pub date: &'a str,
    pub teaser: &'a str,
    pub tags: Vec<Tag<'a>>,
}

#[derive(Content)]
pub struct Page<'a> {
    pub title: &'a str,
    pub articles: Vec<Article<'a>>,
}

#[derive(Content)]
pub struct Hello<'a> {
    pub title: &'a str,
    pub body: &'a str,
}

pub fn hello(data: &Simple) -> Hello<'_> {
    Hello { title: &data.title, body: &data.body }
}

pub fn stocks(data: &[Item]) -> Stocks<'_> {
    let items = data
        .iter()
        .map(|i| Row {
            row_class: &i.row_class,
            index: &i.index,
            negative_class: &i.negative_class,
            value: Stock {
                symbol: &i.value.symbol,
                url: &i.value.url,
                name: &i.value.name,
                price: &i.value.price,
                change: &i.value.change,
                ratio: &i.value.ratio,
            },
        })
        .collect();
    Stocks { items }
}

pub fn page(data: &News) -> Page<'_> {
    let articles = data
        .articles
        .iter()
        .map(|a| Article {
            title: &a.title,
            url: &a.url,
            author: &a.author,
            date: &a.date,
            teaser: &a.teaser,
            tags: a.tags.iter().map(|t| Tag { name: &t.name }).collect(),
        })
        .collect();
    Page { title: &data.title, articles }
}

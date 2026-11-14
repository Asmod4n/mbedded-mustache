// Templates of workloads 1 to 3. The Mustache text is the one of bench/render.cpp; the Jinja text is the same page.
#pragma once

#include <string_view>

namespace probe {

inline constexpr std::string_view kMustache1 = R"TEMPLATE(<title>{{title}}</title><h1>{{ title }}</h1><div>{{{body}}}</div>)TEMPLATE";

inline constexpr std::string_view kMustache2 = R"TEMPLATE(<!DOCTYPE html>
<html>
<head>
	<title>Stock Prices</title>
	<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
	<meta http-equiv="Content-Style-Type" content="text/css" />
	<meta http-equiv="Content-Script-Type" content="text/javascript" />
	<link rel="shortcut icon" href="/images/favicon.ico" />
	<link rel="stylesheet" type="text/css" href="/css/style.css" media="all" />
	<script type="text/javascript" src="/js/util.js"></script>
	<style type="text/css">
		/*<![CDATA[*/
		body {
			color: #333333;
			line-height: 150%;
		}

		thead {
			font-weight: bold;
			background-color: #CCCCCC;
		}

		.odd {
			background-color: #FFCCCC;
		}

		.even {
			background-color: #CCCCFF;
		}

		.minus {
			color: #FF0000;
		}

		/*]]>*/
	</style>

</head>

<body>

	<h1>Stock Prices</h1>

	<table>
		<thead>
    		<tr>
     			<th>#</th>
     			<th>symbol</th>
     			<th>name</th>
     			<th>price</th>
     			<th>change</th>
     			<th>ratio</th>
    		</tr>
   		</thead>
   		<tbody>
			{{#items}}
    		<tr class="{{rowClass}}">
     			<td>{{index}}</td>
    			<td>
      				<a href="/stocks/{{value.symbol}}">{{value.symbol}}</a>
     			</td>
     			<td>
      				<a href="{{value.url}}">{{value.name}}</a>
     			</td>
     			<td>
      				<strong>{{value.price}}</strong>
     			</td>
			     <td{{negativeClass}}>{{value.change}}</td>
			     <td{{negativeClass}}>{{value.ratio}}</td>
    		</tr>
			{{/items}}
   		</tbody>
	</table>

</body>
</html>
)TEMPLATE";

inline constexpr std::string_view kMustache3 = R"TEMPLATE(<!DOCTYPE html>
<html>
<head><title>{{title}}</title></head>
<body>
<h1>{{title}}</h1>
{{#articles}}
<article>
  <h2><a href="{{url}}">{{title}}</a></h2>
  <p class="meta">{{author}} - {{date}}</p>
  <p>{{teaser}}</p>
  <ul>{{#tags}}<li>{{name}}</li>{{/tags}}</ul>
</article>
{{/articles}}
</body>
</html>
)TEMPLATE";

inline constexpr std::string_view kJinja1 = R"TEMPLATE(<title>{{ title }}</title><h1>{{ title }}</h1><div>{{ body }}</div>)TEMPLATE";

inline constexpr std::string_view kJinja2 = R"TEMPLATE(<!DOCTYPE html>
<html>
<head>
	<title>Stock Prices</title>
	<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
	<meta http-equiv="Content-Style-Type" content="text/css" />
	<meta http-equiv="Content-Script-Type" content="text/javascript" />
	<link rel="shortcut icon" href="/images/favicon.ico" />
	<link rel="stylesheet" type="text/css" href="/css/style.css" media="all" />
	<script type="text/javascript" src="/js/util.js"></script>
	<style type="text/css">
		/*<![CDATA[*/
		body {
			color: #333333;
			line-height: 150%;
		}

		thead {
			font-weight: bold;
			background-color: #CCCCCC;
		}

		.odd {
			background-color: #FFCCCC;
		}

		.even {
			background-color: #CCCCFF;
		}

		.minus {
			color: #FF0000;
		}

		/*]]>*/
	</style>

</head>

<body>

	<h1>Stock Prices</h1>

	<table>
		<thead>
    		<tr>
     			<th>#</th>
     			<th>symbol</th>
     			<th>name</th>
     			<th>price</th>
     			<th>change</th>
     			<th>ratio</th>
    		</tr>
   		</thead>
   		<tbody>
			{% for item in items %}
    		<tr class="{{ item.rowClass }}">
     			<td>{{ item.index }}</td>
    			<td>
      				<a href="/stocks/{{ item.value.symbol }}">{{ item.value.symbol }}</a>
     			</td>
     			<td>
      				<a href="{{ item.value.url }}">{{ item.value.name }}</a>
     			</td>
     			<td>
      				<strong>{{ item.value.price }}</strong>
     			</td>
			     <td{{ item.negativeClass }}>{{ item.value.change }}</td>
			     <td{{ item.negativeClass }}>{{ item.value.ratio }}</td>
    		</tr>
			{% endfor %}
   		</tbody>
	</table>

</body>
</html>
)TEMPLATE";

inline constexpr std::string_view kJinja3 = R"TEMPLATE(<!DOCTYPE html>
<html>
<head><title>{{ title }}</title></head>
<body>
<h1>{{ title }}</h1>
{% for article in articles %}
<article>
  <h2><a href="{{ article.url }}">{{ article.title }}</a></h2>
  <p class="meta">{{ article.author }} - {{ article.date }}</p>
  <p>{{ article.teaser }}</p>
  <ul>{% for tag in article.tags %}<li>{{ tag.name }}</li>{% endfor %}</ul>
</article>
{% endfor %}
</body>
</html>
)TEMPLATE";

}

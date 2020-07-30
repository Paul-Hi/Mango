---
title: Blog
subtitle: Interesting blog entries will be posted here!
layout: wrap
sidebar: blog
url: blog.html
---

<div class="d-flex flex-column justify-content-between sidebar-mango col-lg-1">
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title">{{ page.title }}</h1>
            <p class="card-text">{{ page.subtitle }}</p>
        </div>
    </div>
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title">In Progress</h1>
            <p class="card-text">
                {% include in_progress.html %}
            </p>
        </div>
    </div>
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title">Post History</h1>
            <div class="card-text">
                {% for post in site.posts %}
                    {% assign currentdate = post.date | date: "%Y" %}
                    {% if currentdate != date %}
                        {% unless forloop.first %}</ul>{% endunless %}
                        <h4 id="y{{post.date | date: "%Y"}}">{{ currentdate }}</h4>
                        {% assign date = currentdate %}
                    {% endif %}
                    <a class="mango-card-link d-flex p-2" href="{{ post.url }}">
                        <span class="pl-2"> {{ post.title}} </span>
                        <span class="post-date ml-auto pl-2"> {{ post.date | date: "%b %-d, %Y" }}</span>
                    </a>
                {% endfor %}
            </div>
        </div>
    </div>
</div>

<div class="d-flex flex-column justify-content-between col">
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title">Blog Posts</h1>
            <p class="card-text">
                {% for post in site.posts %}
                    <div class="card mt-3 flex-grow-1">
                        <div class="card-body post-card" style="background-image:url({{ post.fimg | absolute_url }});">
                            <a class="card-title mango-link" href="{{ post.url }}">
                                <h2 class="mt-0">{{ post.title }}</h2>
                            </a>
                            <p class="card-text">
                                <h3>{{ post.subtitle }}</h3>
                                <p>{{ post.excerpt }}</p>
                                <a class="mango-link" href="{{ post.url }}">Read more...</a>
                            </p>
                        </div>
                    </div>
                {% endfor %}
            </p>
        </div>
    </div>
</div>
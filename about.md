---
title: About
subtitle: All the information about Mango!
layout: wrap
sidebar: about
url: about.html
---

<div class="d-flex flex-column justify-content-between sidebar-mango col-lg-1">
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title">{{ page.title }}</h1>
            <p class="card-text">{{ page.subtitle }}</p>
        </div>
    </div>
    <!-- ROADMAP -->
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title">Roadmap</h1>
            <p class="card-text">
            (unordered and incomplete)
                {% include roadmap.html %}
            </p>
        </div>
    </div>
</div>

<div class="d-flex flex-column justify-content-between col">
    <!-- WHAT IS IT -->
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title">What is Mango</h1>
            <p class="card-text">
                Mango is an Open Source Framework. Or at least it should be in the future.
                A playground for people like me.<br><br>
                Just to be clear: This is not the first attempt!
                I plan to implement various features, with main focus on computer graphics.
                A few of them are described below, but there will be added a lot more in the future.
                There should also always be a documentation and tests to ensure cleaner and more
                functional
                code.<br><br>
                If you have suggestions for improvement or find a bug just let me know on <a
                    class="mango-link" href="https://github.com/Paul-Hi/Mango">GitHub</a>.
                Keep in mind that I do this in my spare time, besides studying and working.<br><br>
                For more info have a look at the first blog post
                '<a class="mango-link" href="{{ "/2020/06/09/From-0.html" | relative_url }}">From 0 to 0.5</a>'.
            </p>
        </div>
    </div>
    <!-- FEATURES -->
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title">Features</h1>
            <p class="card-text">
                {% include features.html %}
            </p>
        </div>
    </div>
    <!-- CLICKUP BOARD -->
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title">ClickUp Board</h1>
            <iframe class="clickup-embed"
                src="https://share.clickup.com/b/h/6-17154399-2/ae014866f2d5daa" frameborder="0"
                onmousewheel=""
                style="background: transparent; border: 1px solid #ccc;"></iframe>
        </div>
    </div>
</div>
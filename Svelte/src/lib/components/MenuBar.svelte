<script lang="ts">
</script>

<div class="full-width" role="menubar">
	<ul class="menu">
		<li>
			<span>파일</span>
			<ul class="dropdown">
				<li><span>새로 만들기</span></li>
				<li><span>불러오기</span></li>
				<li><hr /></li>
				<li><span>시뮬레이션</span></li>
			</ul>
		</li>
		<li><span>도구</span></li>
		<li><span>제어기</span></li>
		<li><span>설정</span></li>
	</ul>
</div>

<style>
	.full-width {
		display: block; /* 인라인 요소라면 block으로 변경 */
		width: 100%; /* 부모 너비의 100% 차지 */
		background: #4e83db;
	}

	.menu {
		list-style: none;
		margin: 0;
		padding: 0;
		display: flex;
	}

	.menu > li {
		position: relative;
	}

	ul > li > span {
		display: block;
		padding: 10px 20px;
		color: #fff;
		text-decoration: none;
	}

	ul > li:hover > span {
		color: #000;
		background: #fff;
	}

	/* 드롭다운 숨김 */
	.dropdown {
		display: none;
		position: absolute;
		top: 100%;
		left: 0;

		min-width: 200px;

		list-style: none;
		margin: 0;
		padding: 0;
		background: #4675c7;
	}

	/* 마우스 오버 시 표시 */
	.menu > li:hover .dropdown {
		display: block;
	}
</style>

/*
 * Copyright (C) 2015, 2016 "IoT.bzh"
 * Author "Romain Forlot" <romain.forlot@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/* This is a classic observer design pattern a bit enhanced to be
 able to pull and push info in 2 ways */


 #include <list>
 #include <algorithm>

template<class T>
class Observable;

template<class T>
class Observer
{
protected:
	virtual ~Observer()
	{
		const_iterator_ ite = observableList_.end();

		for(iterator_ itb=observableList_.begin();itb!=ite;++itb)
		{
				(*itb)->delObserver(this);
		}
	}

	std::list<Observable<T>*> observableList_;
	typedef typename std::list<Observable<T>*>::iterator iterator_;
	typedef typename std::list<Observable<T>*>::const_iterator const_iterator_;

public:
	virtual void update(T* observable) = 0;

	void addObservable(Observable<T>* observable)
	{
		for (auto& obs : observableList_)
		{
			if (obs == observable)
				{return;}
		}

		observableList_.push_back(observable);
	}

	void delObservable(Observable<T>* observable)
	{
		const_iterator_ it = std::find(observableList_.begin(), observableList_.end(), observable);
		if(it != observableList_.end())
			{observableList_.erase(it);}
	}
};

template <class T>
class Observable
{
public:
	void addObserver(Observer<T>* observer)
	{
		observerList_.push_back(observer);
		observer->addObservable(this);
	}

	void delObserver(Observer<T>* observer)
	{
		const_iterator_ it = find(observerList_.begin(), observerList_.end(), observer);
		if(it != observerList_.end())
			{observerList_.erase(it);}
	}

	virtual int initialRecursionCheck() = 0;
	virtual int recursionCheck(T* obs) = 0;

	virtual ~Observable()
	{
		iterator_ itb = observerList_.begin();
		const_iterator_ ite = observerList_.end();

		for(;itb!=ite;++itb)
		{
			(*itb)->delObservable(this);
		}
	}

protected:
	std::list<Observer<T>*> observerList_;
	typedef typename std::list<Observer<T>*>::iterator iterator_;
	typedef typename std::list<Observer<T>*>::const_iterator const_iterator_;

	void notify()
	{
		iterator_ itb=observerList_.begin();
		const_iterator_ ite=observerList_.end();

		for(;itb!=ite;++itb)
		{
			(*itb)->update(static_cast<T*>(this));
		}
	}
};
